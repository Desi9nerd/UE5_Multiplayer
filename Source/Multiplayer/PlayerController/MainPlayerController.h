#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/** PlayerController
 *  HUD를 Update 한다.
 */
UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar에 Health/MaxHealth
	void SetHUDScore(float Score); // 점수 매기기
	void SetHUDDefeats(int32 Defeats); // 승리횟수 매기기
	void SetHUDWeaponAmmo(int32 Ammo); // 총알 수 업데이트해서 띄우기
	void SetHUDCarriedAmmo(int32 Ammo);// 최대 총알 수 띄우기
	void SetHUDMatchCountdown(float CountdownTime); // 남은시간 띄우기
	virtual void OnPossess(APawn* InPawn) override; // possed된 Pawn에 접근하는 함수
	virtual float GetServerTime(); // Synced된 Server world clock를 리턴하는 함수
	virtual void ReceivedPlayer() override; // 가능한 빨리 server clock을 Sync
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted(); // 경기 시작 시 Announcement 위젯 안 보이게 하기

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit(); // 체력, 점수, 승패 초기화

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

private:
	TObjectPtr<class AMainHUD> MainHUD;

	float MatchTime = 120.0f; // 경기시간
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState) // Client들에게 Replicated 되도록 설정.
	FName MatchState; // GameMode.h의 이름이 같은 MatchState이 있다.

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};
