#pragma once

#include "CoreMinimal.h"
#include "TeamGameMode.h"
#include "CTFGameMode.generated.h"

/** Capture The Flag GameMode
 *  ATeamGameMode의 자식 클래스
 */
UCLASS()
class MULTIPLAYER_API ACTFGameMode : public ATeamGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(class ABaseCharacter* ElimmedCharacter, class AMainPlayerController* VictimController, AMainPlayerController* AttackerController) override;
	void FlagCaptured(class AFlag* Flag, class AFlagZone* Zone);
};
