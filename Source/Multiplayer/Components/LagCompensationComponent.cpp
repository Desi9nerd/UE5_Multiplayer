#include "LagCompensationComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Multiplayer/Multiplayer.h"
#include "DrawDebugHelpers.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame,
	const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;

		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.0f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.0f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent; // BoxExtent�� ������� �ʴ´�

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo); // InterpBoxInfo ������ �����Ͽ� HitBoxInfo�� �ִ´�
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
	ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame); // HitCollisionBoxes ���
	MoveBoxes(HitCharacter, Package);  // HitCollisionBoxes �̵�
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// �Ӹ� HitCollisionBox�� ���� �浹ó�� �� �� �ֵ��� ���ش�.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox); // LineTrace�� ��弦 �浹 �˻�

		if (ConfirmHitResult.bBlockingHit) // ��弦�� ���� ���, �ٷ� return 
		{
			ResetHitBoxes(HitCharacter, CurrentFrame); // HitCollisionBoxes �ʱ�ȭ
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{ true, true };
		}
		else // ��弦�� �ƴ� ���, ������ HitCollsionBoxes���� LineTrace�� �浹 �˻�
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}
			}

			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

			if (ConfirmHitResult.bBlockingHit) // �浹 O
			{
				ResetHitBoxes(HitCharacter, CurrentFrame); // HitCollisionBoxes �ʱ�ȭ
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

				return FServerSideRewindResult{ true, false };
			}
		}
	}

	//** �浹ó���� �� �� ���. ��弦X. ����X
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame); // HitCollisionBoxes ���
	MoveBoxes(HitCharacter, Package);  // HitCollisionBoxes �̵�
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// �Ӹ� HitCollisionBox�� ���� �浹ó�� �� �� �ֵ��� ���ش�.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);


	FPredictProjectilePathParams PathParams; // Projectile Path ��꿡 �ʿ��� ������ ��� ����
	PathParams.bTraceWithCollision = true; // �� Trace�� Hit Event�� �߻��ϵ��� true ����
	PathParams.MaxSimTime = MaxRecordTime; // �߻�ü�� ���߿� �ӹ� �� �ִ� �ִ�ð�
	PathParams.LaunchVelocity = InitialVelocity; // �߻�ӵ� = ���ʼӵ�
	PathParams.StartLocation = TraceStart; // ������ġ
	PathParams.SimFrequency = 15.0f; // �󵵼�. �������� SphereTrace�� ��������.
	PathParams.ProjectileRadius = 5.0f; // SphereTrace�� ������ ��
	PathParams.TraceChannel = ECC_HitBox; // Multiplayer.h�� #define ������ ä�� ���
	PathParams.ActorsToIgnore.Add(GetOwner()); // Projectile Path Trace ��꿡 ���õǴ� Actor��
	//PathParams.DrawDebugTime = 5.0f; // ����� �����ֱ� ���ӽð�
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration; // DrawDebugTime ���ȸ� �����ֵ��� ����

	FPredictProjectilePathResult PathResult; // Projectile Path �����
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult); // PathParams ������ ProjectilePath�� ����Ͽ� PathResult�� ������� ������Ʈ�Ѵ�.

	if (PathResult.HitResult.bBlockingHit) // ��弦�� ���� ���, �ٷ� return 
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
	else // ��弦�� �ƴ� ���, ������ HitCollsionBoxes���� LineTrace�� �浹 �˻�
	{
		for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
		if (PathResult.HitResult.bBlockingHit)
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, false };
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : FramePackages) // ���� ó��
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}


	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame); // HitCollisionBoxes ���
		MoveBoxes(Frame.Character, Frame); // HitCollisionBoxes �̵�
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);

		CurrentFrames.Add(CurrentFrame);
	}

	//** �Ӹ� HitCollisionBox�� ���� �浹ó�� üũ �� �� �ֵ��� ���ش�.
	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	TWeakObjectPtr<UWorld> World = GetWorld();

	//** �浹üũ: ��弦 
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World.IsValid())
		{
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox); // LineTrace�� ��弦 �浹 �˻�

			TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(ConfirmHitResult.GetActor()); // ��弦�� ���� ���, ĳ���� ����
			if (IsValid(BaseCharacter)) // ��弦 ���� ĳ���Ͱ� �ִٸ�
			{
				if (ShotgunResult.HeadShots.Contains(BaseCharacter))
				{	// ù ��弦 ������ ��弦�� ++
					ShotgunResult.HeadShots[BaseCharacter]++;
				}
				else
				{	// ù ��弦
					ShotgunResult.HeadShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}

	//** head ���� HitCollisionBoxes���� Ȱ��ȭ. ��弦�� head HitCollsionBox�� ��Ȱ��ȭ
	for (auto& Frame : FramePackages)
	{
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // head�� HitCollisionBox�� ��Ȱ��ȭ
	}

	//** �浹 üũ: �ٵ�. head �� ������ ���� HitCollisionBoxes���� �浹 üũ
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		if (World.IsValid())
		{
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
			TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(ConfirmHitResult.GetActor());
			if (IsValid(BaseCharacter))
			{
				if (ShotgunResult.BodyShots.Contains(BaseCharacter))
				{	// ù �ٵ� ������ ��弦�� ++
					ShotgunResult.BodyShots[BaseCharacter]++;
				}
				else
				{	// ù �ٵ�
					ShotgunResult.BodyShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}

	//** HitCollissionBoxes �ʱ�ȭ
	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;

	// HitCharacter�� HitCollisionBoxes ���� ������Ʈ �� �ֱ�
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr) // ���� �ִٸ�
		{
			// BoxInfo ������ HitCollisionBoxes�� ��ġ,ȸ��,�ڽ�Extent ���� ���
			// OutFramePackage�� HitBoxInfo�� Key, Value�� �־� �߰��Ѵ�
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr) // ���� �ִٸ�
		{
			// ��ġ, ȸ��, �ڽ�Extent �� �����Ͽ� ������Ʈ
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color) // Package ���� HitBoxInfo ���
{
	// SaveFramePackage���� ���ŵ� ���� ���
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color, false, 4.0f);
	}
}

// HitCharacter: �ǰݽ�Ű�� ĳ����. LineTrace�� ������ϱ� ���� TraceStart�� HitLocation ���. HitTime: Server Rewind�� ���� �ð�
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime) // Server-side Rewinding Time �˰���
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABaseCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABaseCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABaseCharacter* HitCharacter : HitCharacters) // HitCharacters�� üũ
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();

	// Frame package that we check to verify a hit. �ǰ� Ȯ���� ���� Frame package. 
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// HitCharacter�� Frame history
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime)
	{
		// �ʹ� ������. too far back: too laggy to do Server-side Rewind
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older->GetValue().Time > HitTime) // is Older still younger than HitTime?
	{
		// OlderTime < HitTime < YoungerTime �� ������ while�� ����
		if (Older->GetNextNode() == nullptr) break;

		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) // HitTime�� FramePackage�� Time�� ��Ȯ�� ��ġ�� Ȯ���� ���� ������ üũ
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		// OlderFrame�� YoungerFrame ���� ����
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;

	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime); // ServerSideRewind �˰������� Hit ���� �Ǻ�

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon()) // Hit �Ǿ��ٸ�
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage(); // ��弦, �ٵ� ������ �� �´°� ����

		// ������ ����
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage, // ��弦 or �ٵ� ������
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		); 
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon()) // Hit �Ǿ��ٸ�
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		// ������ ����
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABaseCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || HitCharacter->GetEquippedWeapon() == nullptr || Character == nullptr) continue; // ���� ó��

		float TotalDamage = 0.0f; // ������ ������ �ѷ�
		if (Confirm.HeadShots.Contains(HitCharacter)) // ��弦
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter)) // �ٵ�
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		//** ������ ����
		UGameplayStatics::ApplyDamage(HitCharacter, TotalDamage, Character->Controller, HitCharacter->GetEquippedWeapon(), UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || Character->HasAuthority() == false) return; // Client(=Server�� �ƴ϶��)�� ����

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame); // FrameHistory�� �߰�
	}
	else
	{
		// HistoryLength = Newest Frame Package Time - Oldest Frame Package Time
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail()); // FrameHistory���� ����
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;// HistoryLength ������Ʈ
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame); // FrameHistory�� �߰�

		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetOwner()) : Character;
	if (IsValid(Character))
	{
		Package.Time = GetWorld()->GetTimeSeconds(); // Server Time ���
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation); // Key���� BoxInformation�� ��´�.
		}
	}
}
