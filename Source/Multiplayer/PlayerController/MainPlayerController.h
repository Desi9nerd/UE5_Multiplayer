#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "MainPlayerController.generated.h"

class AMultiplayerGameMode;
class UReturnToMainMenu;
class AMainHUD;
class UCharacterOverlay;

/** PlayerController
 *  HUD�� Update �Ѵ�.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void HideTeamScores(); // Team ���� �����
	void InitTeamScores(); // Team ���� �ʱ�ȭ
	void SetHUDRedTeamScore(int32 RedScore); // RedTeam ���� ����
	void SetHUDBlueTeamScore(int32 BlueScore);// BlueTeam ���� ����
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar�� Health/MaxHealth
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score); // ���� �ű��
	void SetHUDDefeats(int32 Defeats); // �¸�Ƚ�� �ű��
	void SetHUDWeaponAmmo(int32 Ammo); // �Ѿ� �� ������Ʈ�ؼ� ����
	void SetHUDCarriedAmmo(int32 Ammo);// �ִ� �Ѿ� �� ����
	void SetHUDMatchCountdown(float CountdownTime); // �����ð� ����
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades); // ����ź �� ����
	void SetHUDWeaponImage(EWeaponType weaponType); // ����Ÿ�� �̹��� ����
	virtual void OnPossess(APawn* InPawn) override; // possed�� Pawn�� �����ϴ� �Լ�
	virtual float GetServerTime(); // Synced�� Server world clock�� �����ϴ� �Լ�
	virtual void ReceivedPlayer() override; // ������ ���� server clock�� Sync
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false); // ��� ���� �� Announcement ���� �� ���̰� �ϱ�
	void HandleCooldown(); // ��� ���� �� Announcement ���� ���̰� �ϱ�
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim); // GameMode���� ����(=Server���� call�ȴ�)

	float SingleTripTime = 0.0f; // SingleTripTime = 0.5f * RoundTripTime;
	FHighPingDelegate HighPingDelegate;

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit(); // ü��, ����, ����, ����ź �ʱ�ȭ
	virtual void SetupInputComponent() override;

	//** Announcement ���� ����
	FString GetInfoText(const TArray<class AMultiplayerPlayerState*>& Players);
	FString GetTeamsInfoText(class AMultiplayerGameState* MultiplayerGameState);

	//** Server�� Client ������ Sync Time	
	// Request �޾��� �� Client�� Time�� �����Ͽ� ���� Server Time�� ��û�ϴ� �Լ�.
	UFUNCTION(Server, Reliable) // Server RPC, client->Server
	void ServerRequestServerTime(float TimeOfClientRequest);
	
	// ServerRequestServerTime�� �����Ͽ� Client�� ���� Server Time�� �����ϴ� �Լ�
	UFUNCTION(Client, Reliable) // Client RPC
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.0f; // Client�� Server�� �ð���

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.0f; // Server Time�� Sync�ϴ� �ֱ�

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime); // �� TimeSyncFrequency ���� Server Time�� Sync�ϴ� �Լ�

	UFUNCTION(Server, Reliable) // Server RPC
	void ServerCheckMatchState(); // ���� ���� �ð��� �����ϴ� �Լ�. GameMode�� ���� �־��ְ� Client RPC�� ���ϴ� �Լ�

	UFUNCTION(Client, Reliable) // Client RPC
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime); // Client�� ���ӿ� �������� Client���� MatchState�� �˸��� �Լ�

	void CheckPing(float DeltaTime); // Ping üũ
	void HighPingWarning(); // High Ping ���(�̹��� ����)
	void StopHighPingWarning(); // High Ping ��� ���߱�(�̹��� �� ����)

	void ShowReturnToMainMenu(); // Ű �Է� �� ReturnToMainMenu â ����

	UFUNCTION(Client, Reliable) // Client RPC
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim); // ������, �ǰ����� �״� �÷��̾ ȭ�鿡 �ؽ�Ʈ�� ����

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	UFUNCTION()
	void OnRep_ShowTeamScores();


private:
	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY(ReplicatedUsing = OnRep_MatchState) // Client�鿡�� Replicated �ǵ��� ����.
	FName MatchState; // GameMode.h�� �̸��� ���� MatchState�� �ִ�.

	UPROPERTY()
	TObjectPtr<AMultiplayerGameMode> MultiplayerGameMode;

	UPROPERTY()
	TObjectPtr<AMainHUD> MainHUD;
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;

	//** Return to Main Menu
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	UPROPERTY()
	TObjectPtr<UReturnToMainMenu> ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	float LevelStartingTime = 0.0f; // ���ӷ����ʿ� �� �ð�
	float MatchTime = 0.0f;		// ��� �ð�
	float WarmupTime = 0.0f;	// ��� ���� �� ��� �ð�
	float CooldownTime = 0.0f;  // ��� ���� �� ��� �ð�
	uint32 CountdownInt = 0;

	bool bInitializeHealth = false;
	bool bInitializeScore = false;
	bool bInitializeDefeats = false;
	bool bInitializeGrenades = false;
	bool bInitializeShield = false;
	bool bInitializeCarriedAmmo = false;
	bool bInitializeWeaponAmmo = false;
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	float HUDCarriedAmmo;
	float HUDWeaponAmmo;
	int32 HUDDefeats;
	int32 HUDGrenades;

	//********************************************************
	//** Ping ���� ������ + �Լ�
	UFUNCTION(Server, Reliable) // Server RPC
	void ServerReportPingStatus(bool bHighPing);
	float HighPingRunningTime = 0.0f;
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.0f;
	float PingAnimationRunningTime = 0.0f;
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.0f;
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.0f;
	//********************************************************
};
