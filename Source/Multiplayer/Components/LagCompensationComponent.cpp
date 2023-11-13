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
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent; // BoxExtent는 변경되지 않는다

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo); // InterpBoxInfo 정보를 갱신하여 HitBoxInfo에 넣는다
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
	ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame); // HitCollisionBoxes 기록
	MoveBoxes(HitCharacter, Package);  // HitCollisionBoxes 이동
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// 머리 HitCollisionBox를 먼저 충돌처리 할 수 있도록 해준다.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox); // LineTrace로 헤드샷 충돌 검사

		if (ConfirmHitResult.bBlockingHit) // 헤드샷이 나온 경우, 바로 return 
		{
			ResetHitBoxes(HitCharacter, CurrentFrame); // HitCollisionBoxes 초기화
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{ true, true };
		}
		else // 헤드샷이 아닌 경우, 나머지 HitCollsionBoxes들을 LineTrace로 충돌 검사
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

			if (ConfirmHitResult.bBlockingHit) // 충돌 O
			{
				ResetHitBoxes(HitCharacter, CurrentFrame); // HitCollisionBoxes 초기화
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

				return FServerSideRewindResult{ true, false };
			}
		}
	}

	//** 충돌처리가 안 된 경우. 헤드샷X. 몸샷X
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame); // HitCollisionBoxes 기록
	MoveBoxes(HitCharacter, Package);  // HitCollisionBoxes 이동
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// 머리 HitCollisionBox를 먼저 충돌처리 할 수 있도록 해준다.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);


	FPredictProjectilePathParams PathParams; // Projectile Path 계산에 필요한 값들을 담는 변수
	PathParams.bTraceWithCollision = true; // 쏜 Trace의 Hit Event가 발생하도록 true 설정
	PathParams.MaxSimTime = MaxRecordTime; // 발사체가 공중에 머물 수 있는 최대시간
	PathParams.LaunchVelocity = InitialVelocity; // 발사속도 = 최초속도
	PathParams.StartLocation = TraceStart; // 시작위치
	PathParams.SimFrequency = 15.0f; // 빈도수. 높을수록 SphereTrace가 많아진다.
	PathParams.ProjectileRadius = 5.0f; // SphereTrace의 반지름 값
	PathParams.TraceChannel = ECC_HitBox; // Multiplayer.h에 #define 정의한 채널 사용
	PathParams.ActorsToIgnore.Add(GetOwner()); // Projectile Path Trace 계산에 무시되는 Actor들
	//PathParams.DrawDebugTime = 5.0f; // 디버그 보여주기 지속시간
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration; // DrawDebugTime 동안만 보여주도록 설정

	FPredictProjectilePathResult PathResult; // Projectile Path 결과값
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult); // PathParams 값으로 ProjectilePath를 계산하여 PathResult로 결과값을 업데이트한다.

	if (PathResult.HitResult.bBlockingHit) // 헤드샷이 나온 경우, 바로 return 
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
	else // 헤드샷이 아닌 경우, 나머지 HitCollsionBoxes들을 LineTrace로 충돌 검사
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
	for (auto& Frame : FramePackages) // 예외 처리
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}


	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame); // HitCollisionBoxes 기록
		MoveBoxes(Frame.Character, Frame); // HitCollisionBoxes 이동
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);

		CurrentFrames.Add(CurrentFrame);
	}

	//** 머리 HitCollisionBox를 먼저 충돌처리 체크 할 수 있도록 해준다.
	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	TWeakObjectPtr<UWorld> World = GetWorld();

	//** 충돌체크: 헤드샷 
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World.IsValid())
		{
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox); // LineTrace로 헤드샷 충돌 검사

			TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(ConfirmHitResult.GetActor()); // 헤드샷이 나온 경우, 캐스팅 성공
			if (IsValid(BaseCharacter)) // 헤드샷 맞은 캐릭터가 있다면
			{
				if (ShotgunResult.HeadShots.Contains(BaseCharacter))
				{	// 첫 헤드샷 이후의 헤드샷은 ++
					ShotgunResult.HeadShots[BaseCharacter]++;
				}
				else
				{	// 첫 헤드샷
					ShotgunResult.HeadShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}

	//** head 외의 HitCollisionBoxes들을 활성화. 헤드샷용 head HitCollsionBox는 비활성화
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
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // head의 HitCollisionBox는 비활성화
	}

	//** 충돌 체크: 바디샷. head 외 나머지 몸의 HitCollisionBoxes들의 충돌 체크
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
				{	// 첫 바디샷 이후의 헤드샷은 ++
					ShotgunResult.BodyShots[BaseCharacter]++;
				}
				else
				{	// 첫 바디샷
					ShotgunResult.BodyShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}

	//** HitCollissionBoxes 초기화
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

	// HitCharacter의 HitCollisionBoxes 정보 업데이트 후 넣기
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr) // 값이 있다면
		{
			// BoxInfo 변수에 HitCollisionBoxes의 위치,회전,박스Extent 값을 담고
			// OutFramePackage의 HitBoxInfo에 Key, Value로 넣어 추가한다
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
		if (HitBoxPair.Value != nullptr) // 값이 있다면
		{
			// 위치, 회전, 박스Extent 값 설정하여 업데이트
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

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color) // Package 내의 HitBoxInfo 사용
{
	// SaveFramePackage에서 갱신된 정보 사용
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color, false, 4.0f);
	}
}

// HitCharacter: 피격시키는 캐릭터. LineTrace를 재생산하기 위해 TraceStart와 HitLocation 사용. HitTime: Server Rewind를 위한 시간
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime) // Server-side Rewinding Time 알고리즘
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
	for (ABaseCharacter* HitCharacter : HitCharacters) // HitCharacters들 체크
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

	// Frame package that we check to verify a hit. 피격 확인을 위한 Frame package. 
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// HitCharacter의 Frame history
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime)
	{
		// 너무 오래됨. too far back: too laggy to do Server-side Rewind
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
		// OlderTime < HitTime < YoungerTime 될 때까지 while문 진행
		if (Older->GetNextNode() == nullptr) break;

		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) // HitTime이 FramePackage의 Time과 정확히 일치할 확률은 극히 낮지만 체크
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		// OlderFrame과 YoungerFrame 사이 보간
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;

	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime); // ServerSideRewind 알고리즘으로 Hit 여부 판별

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon()) // Hit 되었다면
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage(); // 헤드샷, 바디샷 데미지 중 맞는거 담음

		// 데미지 전달
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage, // 헤드샷 or 바디샷 데미지
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		); 
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon()) // Hit 되었다면
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		// 데미지 전달
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
		if (HitCharacter == nullptr || HitCharacter->GetEquippedWeapon() == nullptr || Character == nullptr) continue; // 예외 처리

		float TotalDamage = 0.0f; // 전달할 데미지 총량
		if (Confirm.HeadShots.Contains(HitCharacter)) // 헤드샷
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter)) // 바디샷
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		//** 데미지 전달
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
	if (Character == nullptr || Character->HasAuthority() == false) return; // Client(=Server가 아니라면)면 리턴

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame); // FrameHistory에 추가
	}
	else
	{
		// HistoryLength = Newest Frame Package Time - Oldest Frame Package Time
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail()); // FrameHistory에서 삭제
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;// HistoryLength 업데이트
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame); // FrameHistory에 추가

		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetOwner()) : Character;
	if (IsValid(Character))
	{
		Package.Time = GetWorld()->GetTimeSeconds(); // Server Time 기록
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation); // Key값과 BoxInformation을 담는다.
		}
	}
}
