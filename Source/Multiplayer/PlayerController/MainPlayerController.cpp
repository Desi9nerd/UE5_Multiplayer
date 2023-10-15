#include "MainPlayerController.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer//HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Multiplayer/Character/BaseCharacter.h"

void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MainHUD = Cast<AMainHUD>(GetHUD());
}

void AMainPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime(); // HUD�� ǥ�õǴ� �ð��� �� ƽ �����Ѵ�.
	CheckTimeSync(DeltaTime); // �� TimeSyncFrequency ���� Server Time�� Sync�Ѵ�.
}

void AMainPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime; // Tick������ DeltaTime�� TimeSyncRunningTime�� ����Ѵ�.

	// Client && TimeSyncRunningTime��  Server Time�� Sync�ϴ� �ֱ��� TimeSyncFrequency���� Ŀ����
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds()); // Server Time�� ��û
		TimeSyncRunningTime = 0.0f; // ����
	}
}

void AMainPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(InPawn);
	if (BaseCharacter.IsValid())
	{
		SetHUDHealth(BaseCharacter->GetHealth(), BaseCharacter->GetMaxHealth());
	}
}

void AMainPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->HealthBar &&
		MainHUD->CharacterOverlay->HealthText;

	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		MainHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		MainHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AMainPlayerController::SetHUDScore(float Score) // ���� �ű��
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score)); //Score float���� FloorToInt�� intȭ �� Fstring��ȯ
		MainHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));//CharacterOverlay ���� ScoreAmount��� UTextBlock������ ScoreText(=SetHUDScore�� ���� float Score�� string���� ��ȯ�� ��)�� ����
	}
}

void AMainPlayerController::SetHUDDefeats(int32 Defeats) // �¸�Ƚ�� �ű��
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		MainHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void AMainPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo); // int�� Ammo ���� FString���� ��ȯ �� AmmoText ������ ����
		MainHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AMainPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		MainHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AMainPlayerController::SetHUDMatchCountdown(float CountdownTime) // ���� �ð� ����
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//%02d: ���ڸ����ڷ� ���
		MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AMainPlayerController::SetHUDTime() // HUD�� �ð� ����
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}

	CountdownInt = SecondsLeft;
}

void AMainPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // Server�� ���� Time
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt); // Server�� ���� Time�� ������
}

void AMainPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // Client�� Server�� ������ �ٽ� Client�� ���ƿ� ������ �ҿ�� �ð� = Client�� ���� Time - Client�� ��û�� ��� Time
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime); // ���� Server Time = Server�� Client�κ��� Time ��û�� ���� �ð� + (0.5f * RoundTripTime) // 0.5f * RoundTripTime ���� �ٻ簪�̴�. �ٻ簪�� ����ص� ����� ����ϱ� ������ ���⼭�� �ٻ簪�� ����ߴ�.
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds(); // Client�� Server�� �ð��� = (������ ����)���� Server Time - ���� Client Time
}

float AMainPlayerController::GetServerTime() // Synced�� Server world clock�� �����ϴ� �Լ�
{
	if (HasAuthority()) // ��û����� Server�� ���
		return GetWorld()->GetTimeSeconds();
	else // ��û����� Client�� ���
		return GetWorld()->GetTimeSeconds() + ClientServerDelta; //���� �ð� + Client�� Server�� �ð���
}

void AMainPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds()); // Server Time�� ��û�Ѵ�. ���� Client�� Time�� ������ �ѱ��.
	}
}