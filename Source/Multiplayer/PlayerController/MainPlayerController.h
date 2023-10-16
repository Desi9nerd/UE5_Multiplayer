#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/** PlayerController
 *  HUD�� Update �Ѵ�.
 */
UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar�� Health/MaxHealth
	void SetHUDScore(float Score); // ���� �ű��
	void SetHUDDefeats(int32 Defeats); // �¸�Ƚ�� �ű��
	void SetHUDWeaponAmmo(int32 Ammo); // �Ѿ� �� ������Ʈ�ؼ� ����
	void SetHUDCarriedAmmo(int32 Ammo);// �ִ� �Ѿ� �� ����
	void SetHUDMatchCountdown(float CountdownTime); // �����ð� ����
	void SetHUDAnnouncementCountdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override; // possed�� Pawn�� �����ϴ� �Լ�
	virtual float GetServerTime(); // Synced�� Server world clock�� �����ϴ� �Լ�
	virtual void ReceivedPlayer() override; // ������ ���� server clock�� Sync
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted(); // ��� ���� �� Announcement ���� �� ���̰� �ϱ�
	void HandleCooldown(); // ��� ���� �� Announcement ���� ���̰� �ϱ�

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit(); // ü��, ����, ���� �ʱ�ȭ

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

private:
	TObjectPtr<class AMainHUD> MainHUD;
	UPROPERTY()
	class AMultiplayerGameMode* MultiplayerGameMode;

	float LevelStartingTime = 0.0f; // ���ӷ����ʿ� �� �ð�
	float MatchTime = 0.0f;		// ��� �ð�
	float WarmupTime = 0.0f;	// ��� ���� �� ��� �ð�
	float CooldownTime = 0.0f;  // ��� ���� �� ��� �ð�
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState) // Client�鿡�� Replicated �ǵ��� ����.
	FName MatchState; // GameMode.h�� �̸��� ���� MatchState�� �ִ�.

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
