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
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar�� Health/MaxHealth
	void SetHUDScore(float Score); // ���� �ű��
	void SetHUDDefeats(int32 Defeats); // �¸�Ƚ�� �ű��
	void SetHUDWeaponAmmo(int32 Ammo); // �Ѿ� �� ������Ʈ�ؼ� ����
	void SetHUDCarriedAmmo(int32 Ammo);// �ִ� �Ѿ� �� ����
	void SetHUDMatchCountdown(float CountdownTime); // �����ð� ����
	virtual void OnPossess(APawn* InPawn) override; // possed�� Pawn�� �����ϴ� �Լ�
	virtual float GetServerTime(); // Synced�� Server world clock�� �����ϴ� �Լ�
	virtual void ReceivedPlayer() override; // ������ ���� server clock�� Sync

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

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

private:
	TObjectPtr<class AMainHUD> MainHUD;

	float MatchTime = 120.0f; // ���ð�
	uint32 CountdownInt = 0;
};
