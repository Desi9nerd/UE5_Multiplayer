#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Multiplayer/EnumTypes/ETeam.h"
#include "TeamPlayerStart.generated.h"

/** 게임 시작 시 플레이어의 팀 설정 역할을 수행하는 PlayerState
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
