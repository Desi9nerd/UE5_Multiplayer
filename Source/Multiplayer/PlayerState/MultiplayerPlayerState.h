#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerPlayerState.generated.h"

/**
 *  PlayerState�� ����� �޾� �������Ͽ� ��������� �Ѵ�.
 */
UCLASS()
class MULTIPLAYER_API AMultiplayerPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override; // APlayerState�� ���ǵ� �Լ� ������

	//** Replication notifies
	virtual void OnRep_Score() override; // APlayerState���� UFUNCTION ����Ǿ� �ִ�.
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount); // ���� ���ϱ�
	void AddToDefeats(int32 DefeatsAmount); // �¸�Ƚ�� ���ϱ�

private:
	TWeakObjectPtr<class ABaseCharacter> Character;
	TWeakObjectPtr<class AMainPlayerController> Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
