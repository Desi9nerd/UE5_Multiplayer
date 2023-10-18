#include "MainPlayerController.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer//HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/GameMode/MultiplayerGameMode.h"// GameMode�� MatchState�� ����ϱ� ���� ����߰�
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Multiplayer/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h" //UGameplayStatics::GetGameMode() ����ϱ� ���� �߰�
#include "Multiplayer/Components/CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"

void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MainHUD = Cast<AMainHUD>(GetHUD());

	ServerCheckMatchState();
}

void AMainPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainPlayerController, MatchState); // replicated �ǵ��� MatchState ���
}

void AMainPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime(); // HUD�� ǥ�õǴ� �ð��� �� ƽ �����Ѵ�.
	CheckTimeSync(DeltaTime); // �� TimeSyncFrequency ���� Server Time�� Sync�Ѵ�.
	PollInit(); // ü��, ����, ���� �ʱ�ȭ
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

void AMainPlayerController::ServerCheckMatchState_Implementation() // Server RPC
{
	TWeakObjectPtr<AMultiplayerGameMode> GameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode.IsValid())
	{
		// MultiplayerGameMode.h�� ���� �����ٰ� �־��ش�.
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
	// MultiplayerGameMode.h�� ���� �����ٰ� �־��ش�.
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);

	if (MainHUD && MatchState == MatchState::WaitingToStart) // MatchState�� WaitingToStart���
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
	else // HUD�� ���ٸ�
	{	
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
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
	else // HUD�� ���ٸ�
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
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
	else // HUD�� ���ٸ�
	{
		bInitializeCharacterOverlay = true;
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
		if (CountdownTime < 0.0f) // CountdownTime�� ������ �ؽ�Ʈ�� �� ���̰� ���ش�. 
		{
			MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//%02d: ���ڸ����ڷ� ���
		MainHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AMainPlayerController::SetHUDAnnouncementCountdown(float CountdownTime) // ��� ���۱��� ���� �ð� ����
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;

	bool bHUDValid = MainHUD &&
		MainHUD->Announcement &&
		MainHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.0f) // CountdownTime�� ������ �ؽ�Ʈ�� �� ���̰� ���ش�. 
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

void AMainPlayerController::SetHUDTime() // HUD�� �ð� ����
{
	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart) // ��� ���� �� ���ð�
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime; // ��� ���� �� ��� �ð� - ó������ ���ݱ��� �ð� + ���ӷ����ʿ� �� �ð�
	else if (MatchState == MatchState::InProgress) // ��� ���� �� ���ð�
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime + MatchTime;
	else if (MatchState == MatchState::Cooldown) // ��� ���� �� ���ð�
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
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) // ��� ���� �� ���ð� or ��� ���� �� ���ð�
		{
			SetHUDAnnouncementCountdown(TimeLeft); // Announcement Countdown
		}
		if (MatchState == MatchState::InProgress) // ��� ���� �� ���ð�
		{
			SetHUDMatchCountdown(TimeLeft); // ��� Countdown
		};
	}

	CountdownInt = SecondsLeft;
}

void AMainPlayerController::PollInit() // ü��, ����, ���� �ʱ�ȭ
{
	if (CharacterOverlay == nullptr)
	{
		if (MainHUD && MainHUD->CharacterOverlay)
		{
			CharacterOverlay = MainHUD->CharacterOverlay;
			if (IsValid(CharacterOverlay))
			{
				// ü��, ����, ���� �ʱ�ȭ
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
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

void AMainPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;  // GameMode���� �ǳ��޴� FName State���� MatchState ����

	if (MatchState == MatchState::InProgress) //GameMode.h ���� MatchState::InProgress
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

void AMainPlayerController::HandleMatchHasStarted() // ��� ���� �� Announcement ���� �� ���̰� �ϱ�
{
	MainHUD = MainHUD == nullptr ? Cast<AMainHUD>(GetHUD()) : MainHUD;
	if (IsValid(MainHUD))
	{
		MainHUD->AddCharacterOverlay();
		if (MainHUD->Announcement)
		{
			MainHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMainPlayerController::HandleCooldown() // ��� ���� �� Announcement ���� ���̰� �ϱ�
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
			MainHUD->Announcement->SetVisibility(ESlateVisibility::Visible); // Announcement ���̰��ϱ�
			FString AnnouncementText("New Match Starts In:"); // �⺻ ���� ����(���߿� ���� �����ϱ�)
			MainHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AMultiplayerGameState* MultiplayerGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
			AMultiplayerPlayerState* MultiplayerPlayerState = GetPlayerState<AMultiplayerPlayerState>();
			if (IsValid(MultiplayerGameState) && IsValid(MultiplayerPlayerState))
			{
				TArray<AMultiplayerPlayerState*> TopPlayers = MultiplayerGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0) // ���ڰ� ���� ���
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == MultiplayerPlayerState) // �ڽ��� ����
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1) // ���� �̸� ����
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1) // ���ڰ� �������� ���
				{
					// �ְ� �����ڵ� ����
					InfoTextString = FString("Players tied for the win:\n"); 
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
		BaseCharacter->bDisableGameplay = true; // true�� ĳ���� ������ ����. ���콺 ȸ������ �þ� ȸ���� ����
		BaseCharacter->GetCombat()->FireButtonPressed(false); // �߻� ��ư false
	}
}
