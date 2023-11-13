#include "MainPlayerController.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer//HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/GameMode/MultiplayerGameMode.h"// GameMode의 MatchState을 사용하기 위해 헤더추가
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Multiplayer/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h" //UGameplayStatics::GetGameMode() 사용하기 위해 추가
#include "Multiplayer/Components/CombatComponent.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"
#include "Components/Image.h"
#include "Multiplayer/HUD/ReturnToMainMenu.h"

void AMainPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim) // GameMode에서 실행(=Server에서 call된다)
{
	// 해당 PlayerController를 소유한 Client에만 해당 메시지가 나오도록 하기위해 Client RPC 사용
	ClientElimAnnouncement(Attacker, Victim);
}

void AMainPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim) // Client RPC
{
	TWeakObjectPtr<APlayerState> Self = GetPlayerState<APlayerState>();

	if (IsValid(Attacker) && IsValid(Victim) && Self.IsValid())
	{
		MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;
		if (IsValid(MainHUD))
		{
			if (Attacker == Self && Victim != Self)
			{
				MainHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Attacker != Self && Victim == Self)
			{
				MainHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				MainHUD->AddElimAnnouncement("You", "yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				MainHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}
			MainHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MainHUD = Cast<AMainHUD>(GetHUD());

	ServerCheckMatchState();
}

void AMainPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainPlayerController, MatchState); // replicated 되도록 MatchState 등록
}

void AMainPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime(); // HUD에 표시되는 시간을 매 틱 갱신한다.
	CheckTimeSync(DeltaTime); // 매 TimeSyncFrequency 마다 Server Time을 Sync한다.
	PollInit(); // 체력, 점수, 승패 초기화
	CheckPing(DeltaTime); // Ping 체크
}

void AMainPlayerController::CheckPing(float DeltaTime) // Ping 체크
{
	if (HasAuthority()) return; // Server면 예외처리 리턴

	HighPingRunningTime += DeltaTime; // Tick의 DeltaTime을 변수로 받아 더한다

	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (IsValid(PlayerState))
		{
			if (PlayerState->GetPing() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
			{
				HighPingWarning(); // High Ping 경고(이미지 띄우기)
				PingAnimationRunningTime = 0.0f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}

		HighPingRunningTime = 0.0f;
	}

	bool bHighPingAnimationPlaying =
		MainHUD && MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->HighPingAnimation &&
		MainHUD->CharacterOverlay->IsAnimationPlaying(MainHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying) // High Ping 애니메이션이 재생중이면
	{
		PingAnimationRunningTime += DeltaTime;
		// High Ping 애니메이션을 재생 한 시간이 HighPingDuration로 설정한 시간보다 길어지면
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning(); // High Ping 경고 멈추기(이미지 안 띄우기)
		}
	}
}

// Ping이 너무 높으면 bHighPing변수로 true, 아니면 false가 들어온다. 
void AMainPlayerController::ServerReportPingStatus_Implementation(bool bHighPing) // Server RPC
{
	HighPingDelegate.Broadcast(bHighPing); // bHighPing 변수를 Broadcast 시킨다
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

void AMainPlayerController::ServerCheckMatchState_Implementation() // Server RPC
{
	TWeakObjectPtr<AMultiplayerGameMode> GameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode.IsValid())
	{
		// MultiplayerGameMode.h의 값을 가져다가 넣어준다.
		MatchState = GameMode->GetMatchState();
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AMainPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime) // Client RPC
{
	// MultiplayerGameMode.h의 값을 가져다가 넣어준다.
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);

	if (MainHUD && MatchState == MatchState::WaitingToStart) // MatchState이 WaitingToStart라면
	{
		MainHUD->AddAnnouncement(); // Announcement
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
	else // HUD가 없다면
	{	
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AMainPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->ShieldBar &&
		MainHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		MainHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		MainHUD->CharacterOverlay->HealthText->SetText(FText::FromString(ShieldText));
	}
	else // HUD가 없다면
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
	else // HUD가 없다면
	{
		bInitializeScore = true;
		HUDScore = Score;
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
	else // HUD가 없다면
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
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
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
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
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
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
		if (CountdownTime < 0.0f) // CountdownTime이 음수면 텍스트가 안 보이게 해준다. 
		{
			MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//%02d: 두자리숫자로 출력
		MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AMainPlayerController::SetHUDAnnouncementCountdown(float CountdownTime) // 경기 시작까지 남은 시간 띄우기
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->Announcement &&
		MainHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.0f) // CountdownTime이 음수면 텍스트가 안 보이게 해준다. 
		{
			MainHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MainHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AMainPlayerController::SetHUDGrenades(int32 Grenades)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->GrenadesText;
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		MainHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void AMainPlayerController::SetHUDTime() // HUD에 시간 띄우기
{
	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart) // 경기 시작 전 대기시간
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime; // 경기 시작 전 대기 시간 - 처음부터 지금까지 시간 + 게임레벨맵에 들어간 시간
	else if (MatchState == MatchState::InProgress) // 경기 시작 후 경기시간
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime + MatchTime;
	else if (MatchState == MatchState::Cooldown) // 경기 끝난 후 대기시간
		TimeLeft = WarmupTime  - GetServerTime() + LevelStartingTime + MatchTime + CooldownTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (HasAuthority()) // Server
	{
		MultiplayerGameMode = MultiplayerGameMode == nullptr ? Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this)) : MultiplayerGameMode;
		if (MultiplayerGameMode)
		{
			SecondsLeft = FMath::CeilToInt(MultiplayerGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) // 경기 시작 전 대기시간 or 경기 끝난 후 대기시간
		{
			SetHUDAnnouncementCountdown(TimeLeft); // Announcement Countdown
		}
		if (MatchState == MatchState::InProgress) // 경기 시작 후 경기시간
		{
			SetHUDMatchCountdown(TimeLeft); // 경기 Countdown
		};
	}

	CountdownInt = SecondsLeft;
}

void AMainPlayerController::PollInit() // 체력, 점수, 승패 초기화
{
	if (CharacterOverlay == nullptr)
	{
		if (MainHUD && MainHUD->CharacterOverlay)
		{
			CharacterOverlay = MainHUD->CharacterOverlay;
			if (IsValid(CharacterOverlay))
			{
				// 체력, 실드, 점수, 승패, 소유한 탄창, 장착무기 총알 초기화
				if (bInitializeHealth)	SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)	SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore)	SetHUDScore(HUDScore);
				if (bInitializeDefeats)	SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				// 수류탄 수 초기화
				TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(GetPawn());
				if (BaseCharacter.IsValid() && BaseCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(BaseCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void AMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	// 키 입력 시 ReturnToMainMenu 창 띄우기
	InputComponent->BindAction("Quit", IE_Pressed, this, &AMainPlayerController::ShowReturnToMainMenu);
}

void AMainPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = (false == bReturnToMainMenuOpen); // true면 false, false면 true 변환
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup(); // 메뉴 설정 및 띄우기
		}
		else
		{
			ReturnToMainMenu->MenuTearDown(); // 메뉴 해제
		}
	}
}

void AMainPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // Server의 현재 Time
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt); // Server의 현재 Time를 보낸다
}

void AMainPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // Client가 Server에 보내고 다시 Client로 돌아올 때까지 소요된 시간 = Client의 현재 Time - Client이 요청한 당시 Time
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime; // 현재 Server Time = Server가 Client로부터 Time 요청을 받은 시간 + (0.5f * RoundTripTime).  0.5f * RoundTripTime 값은 근사값이다. 근사값을 사용해도 충분히 비슷하기 때문에 여기서는 근사값을 사용했다.
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

void AMainPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;  // GameMode에서 건내받는 FName State으로 MatchState 설정

	if (MatchState == MatchState::InProgress) //GameMode.h 내의 MatchState::InProgress
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AMainPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AMainPlayerController::HandleMatchHasStarted() // 경기 시작 시 Announcement 위젯 안 보이게 하기
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;
	if (IsValid(MainHUD))
	{
		if (MainHUD->CharacterOverlay == nullptr)
		{
			MainHUD->AddCharacterOverlay();			
		}

		if (MainHUD->Announcement)
		{
			MainHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMainPlayerController::HandleCooldown() // 경기 끝난 후 Announcement 위젯 보이게 하기
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	if (IsValid(MainHUD))
	{
		MainHUD->CharacterOverlay->RemoveFromParent();

		bool bHUDValid = MainHUD->Announcement &&
			MainHUD->Announcement->AnnouncementText &&
			MainHUD->Announcement->InfoText;
		if (bHUDValid)
		{
			MainHUD->Announcement->SetVisibility(ESlateVisibility::Visible); // Announcement 보이게하기
			FString AnnouncementText("NEXT GAME STARTS IN:"); // 기본 문구 띄우기(나중에 여기 수정하기)
			MainHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AMultiplayerGameState* MultiplayerGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
			AMultiplayerPlayerState* MultiplayerPlayerState = GetPlayerState<AMultiplayerPlayerState>();
			if (IsValid(MultiplayerGameState) && IsValid(MultiplayerPlayerState))
			{
				TArray<AMultiplayerPlayerState*> TopPlayers = MultiplayerGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0) // 승자가 없는 경우
				{
					InfoTextString = FString("DRAW");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == MultiplayerPlayerState) // 자신이 승자
				{
					InfoTextString = FString("YOU WIN!");
				}
				else if (TopPlayers.Num() == 1) // 승자 이름 띄우기
				{
					InfoTextString = FString::Printf(TEXT("WINNER: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1) // 승자가 여러명인 경우ㄴ
				{
					// 최고 득점자들 띄우기
					InfoTextString = FString("WINNERS:\n"); 
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				MainHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if(BaseCharacter.IsValid() && BaseCharacter->GetCombat())
	{
		BaseCharacter->bDisableGameplay = true; // true면 캐릭터 움직임 제한. 마우스 회전으로 시야 회전은 가능
		BaseCharacter->GetCombat()->FireButtonPressed(false); // 발사 버튼 false
	}
}

void AMainPlayerController::HighPingWarning() // High Ping 경고(이미지 띄우기)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;
	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->HighPingImage &&
		MainHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		// High Ping 이미지를 Opacity를 1로 만들어 보이게 하고, 깜박깜박하는 애니메이션 재생하기
		MainHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0f); 
		MainHUD->CharacterOverlay->PlayAnimation(MainHUD->CharacterOverlay->HighPingAnimation, 0.0f, 5); // 0초부터 재생. 5번 반복 재생
	}
}

void AMainPlayerController::StopHighPingWarning()  // High Ping 경고 멈추기(이미지 안 띄우기)
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;
	bool bHUDValid = MainHUD &&
		MainHUD->CharacterOverlay &&
		MainHUD->CharacterOverlay->HighPingImage &&
		MainHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		// High Ping 이미지를 Opacity를 0으로 만들어 안 보이게 하고, 애니메이션 재생하고 있으면 멈춰주기
		MainHUD->CharacterOverlay->HighPingImage->SetOpacity(0.0f);
		if (MainHUD->CharacterOverlay->IsAnimationPlaying(MainHUD->CharacterOverlay->HighPingAnimation))
		{
			MainHUD->CharacterOverlay->StopAnimation(MainHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}
