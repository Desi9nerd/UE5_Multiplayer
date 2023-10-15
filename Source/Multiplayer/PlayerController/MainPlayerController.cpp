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

	SetHUDTime(); // HUD에 표시되는 시간을 매 틱 갱신한다.
	CheckTimeSync(DeltaTime); // 매 TimeSyncFrequency 마다 Server Time을 Sync한다.
}

void AMainPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime; // Tick에서의 DeltaTime을 TimeSyncRunningTime에 기록한다.

	// Client && TimeSyncRunningTime가  Server Time을 Sync하는 주기인 TimeSyncFrequency보다 커지면
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds()); // Server Time을 요청
		TimeSyncRunningTime = 0.0f; // 리셋
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

void AMainPlayerController::SetHUDScore(float Score) // 점수 매기기
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score)); //Score float변수 FloorToInt로 int화 후 Fstring변환
		MainHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));//CharacterOverlay 안의 ScoreAmount라는 UTextBlock변수를 ScoreText(=SetHUDScore에 들어온 float Score를 string으로 변환한 값)로 설정
	}
}

void AMainPlayerController::SetHUDDefeats(int32 Defeats) // 승리횟수 매기기
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
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo); // int인 Ammo 값을 FString으로 변환 후 AmmoText 변수에 담음
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

void AMainPlayerController::SetHUDMatchCountdown(float CountdownTime) // 남은 시간 띄우기
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//%02d: 두자리숫자로 출력
		MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AMainPlayerController::SetHUDTime() // HUD에 시간 띄우기
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
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // Server의 현재 Time
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt); // Server의 현재 Time를 보낸다
}

void AMainPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // Client가 Server에 보내고 다시 Client로 돌아올 때까지 소요된 시간 = Client의 현재 Time - Client이 요청한 당시 Time
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime); // 현재 Server Time = Server가 Client로부터 Time 요청을 받은 시간 + (0.5f * RoundTripTime) // 0.5f * RoundTripTime 값은 근사값이다. 근사값을 사용해도 충분히 비슷하기 때문에 여기서는 근사값을 사용했다.
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds(); // Client와 Server의 시간차 = (위에서 구한)현재 Server Time - 현재 Client Time
}

float AMainPlayerController::GetServerTime() // Synced된 Server world clock를 리턴하는 함수
{
	if (HasAuthority()) // 요청대상이 Server인 경우
		return GetWorld()->GetTimeSeconds();
	else // 요청대상이 Client인 경우
		return GetWorld()->GetTimeSeconds() + ClientServerDelta; //현재 시간 + Client와 Server의 시간차
}

void AMainPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds()); // Server Time을 요청한다. 현재 Client의 Time을 변수로 넘긴다.
	}
}