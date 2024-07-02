#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Multiplayer/EnumTypes/ETeam.h"
#include "TeamPlayerStart.generated.h"

/** ���� ���� �� �÷��̾��� �� ���� ������ �����ϴ� PlayerState
 * 
 */
UCLASS()
class MULTIPLAYER_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	ETeam Team;
};
