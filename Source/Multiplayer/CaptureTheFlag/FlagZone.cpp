#include "FlagZone.h"
#include "Components/SphereComponent.h"
#include "Multiplayer/Weapon/Flag.h"
#include "Multiplayer/GameMode/CTFGameMode.h"
#include "Multiplayer/Character/BaseCharacter.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
	SetRootComponent(ZoneSphere);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TWeakObjectPtr<AFlag> OverlappingFlag = Cast<AFlag>(OtherActor);

	if (OverlappingFlag.IsValid() && OverlappingFlag->GetTeam() != Team)
	{
		TWeakObjectPtr<ACTFGameMode> GameMode = GetWorld()->GetAuthGameMode<ACTFGameMode>();
		if (GameMode.IsValid()) // Server. Client의 경우 nullptr이다
		{
			GameMode->FlagCaptured(OverlappingFlag.Get(), this);
		}
		OverlappingFlag->ResetFlag();
	}
}