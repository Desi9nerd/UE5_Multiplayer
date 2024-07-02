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
 *  HUD를 Update 한다.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void HideTeamScores(); // Team 점수 숨기기
	void InitTeamScores(); // Team 점수 초기화
	void SetHUDRedTeamScore(int32 RedScore); // RedTeam 점수 띄우기
	void SetHUDBlueTeamScore(int32 BlueScore);// BlueTeam 점수 띄우기
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar에 Health/MaxHealth
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score); // 점수 매기기
	void SetHUDDefeats(int32 Defeats); // 승리횟수 매기기
	void SetHUDWeaponAmmo(int32 Ammo); // 총알 수 업데이트해서 띄우기
	void SetHUDCarriedAmmo(int32 Ammo);// 최대 총알 수 띄우기
	void SetHUDMatchCountdown(float CountdownTime); // 남은시간 띄우기
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades); // 수류탄 수 띄우기
	void SetHUDWeaponImage(EWeaponType weaponType); // 무기타입 이미지 띄우기
	virtual void OnPossess(APawn* InPawn) override; // possed된 Pawn에 접근하는 함수
	virtual float GetServerTime(); // Synced된 Server world clock를 리턴하는 함수
	virtual void ReceivedPlayer() override; // 가능한 빨리 server clock을 Sync
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false); // 경기 시작 시 Announcement 위젯 안 보이게 하기
	void HandleCooldown(); // 경기 끝난 후 Announcement 위젯 보이게 하기
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim); // GameMode에서 실행(=Server에서 call된다)

	float SingleTripTime = 0.0f; // SingleTripTime = 0.5f * RoundTripTime;
	FHighPingDelegate HighPingDelegate;

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit(); // 체력, 점수, 승패, 수류탄 초기화
	virtual void SetupInputComponent() override;

	//** Announcement 띄우기 문자
	FString GetInfoText(const TArray<class AMultiplayerPlayerState*>& Players);
	FString GetTeamsInfoText(class AMultiplayerGameState* MultiplayerGameState);

	//** Server와 Client 사이의 Sync Time	
	// Request 받았을 때 Client의 Time을 전달하여 현재 Server Time을 요청하는 함수.
	UFUNCTION(Server, Reliable) // Server RPC, client->Server
	void ServerRequestServerTime(float TimeOfClientRequest);
	
	// ServerRequestServerTime에 응답하여 Client에 현재 Server Time을 보고하는 함수
	UFUNCTION(Client, Reliable) // Client RPC
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.0f; // Client와 Server의 시간차

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.0f; // Server Time을 Sync하는 주기

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime); // 매 TimeSyncFrequency 마다 Server Time을 Sync하는 함수

	UFUNCTION(Server, Reliable) // Server RPC
	void ServerCheckMatchState(); // 게임 관련 시간을 설정하는 함수. GameMode의 값을 넣어주고 Client RPC를 콜하는 함수

	UFUNCTION(Client, Reliable) // Client RPC
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime); // Client가 게임에 들어왔을때 Client에게 MatchState을 알리는 함수

	void CheckPing(float DeltaTime); // Ping 체크
	void HighPingWarning(); // High Ping 경고(이미지 띄우기)
	void StopHighPingWarning(); // High Ping 경고 멈추기(이미지 안 띄우기)

	void ShowReturnToMainMenu(); // 키 입력 시 ReturnToMainMenu 창 띄우기

	UFUNCTION(Client, Reliable) // Client RPC
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim); // 공격자, 피격으로 죽는 플레이어를 화면에 텍스트로 띄우기

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	UFUNCTION()
	void OnRep_ShowTeamScores();


private:
	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY(ReplicatedUsing = OnRep_MatchState) // Client들에게 Replicated 되도록 설정.
	FName MatchState; // GameMode.h의 이름이 같은 MatchState이 있다.

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

	float LevelStartingTime = 0.0f; // 게임레벨맵에 들어간 시간
	float MatchTime = 0.0f;		// 경기 시간
	float WarmupTime = 0.0f;	// 경기 시작 전 대기 시간
	float CooldownTime = 0.0f;  // 경기 끝난 후 대기 시간
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
	//** Ping 관련 변수들 + 함수
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
