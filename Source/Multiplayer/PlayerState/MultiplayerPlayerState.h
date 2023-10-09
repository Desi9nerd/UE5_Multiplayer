#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerPlayerState.generated.h"

/**
 *  PlayerState의 상속을 받아 재정의하여 점수기록을 한다.
 */
UCLASS()
class MULTIPLAYER_API AMultiplayerPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override; // APlayerState에 정의된 함수 재정의

	//** Replication notifies
	virtual void OnRep_Score() override; // APlayerState에서 UFUNCTION 선언되어 있다.
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount); // 점수 더하기
	void AddToDefeats(int32 DefeatsAmount); // 승리횟수 더하기

private:
	TWeakObjectPtr<class ABaseCharacter> Character;
	TWeakObjectPtr<class AMainPlayerController> Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
