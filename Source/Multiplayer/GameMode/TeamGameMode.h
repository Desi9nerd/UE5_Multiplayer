#pragma once

#include "CoreMinimal.h"
#include "MultiplayerGameMode.h"
#include "TeamGameMode.generated.h"

/** 
 * 
 */
UCLASS()
class MULTIPLAYER_API ATeamGameMode : public AMultiplayerGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override; // MultiplayerGameMode 함수 오버라이드

protected:
	virtual void HandleMatchHasStarted() override;
};
