#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

/** Server-side Rewind�� ����
 *  Recording Frame History: ��� Player���� ��ġ�� ���
 */

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;
	UPROPERTY() // �� Bone(=FName)�� �� ������ BoxInformation�� ����� �� ����� ����
	TMap<FName, FBoxInformation> HitBoxInfo;
	UPROPERTY()
	class ABaseCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;
	UPROPERTY()
	bool bHeadShot; 
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()
	// HitCharacter�� �󸶳� ���� �ǰ݉���� �˾ƾߵǱ� ������ TMap ���
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> HeadShots;
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class ABaseCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	//** Hitscan
	FServerSideRewindResult ServerSideRewind(
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime); // Server-side Rewinding Time �˰���

	//** Projectile
	FServerSideRewindResult ProjectileServerSideRewind(
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime); // Server-side Rewinding Time �˰���

	//** Shotgun
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABaseCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime); // Server-side Rewinding Time �˰���

	//** Hitscan
	UFUNCTION(Server, Reliable) // Client���κ��� call�Ǿ� Server���� ����ȴ�.
	void ServerScoreRequest(
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime, 
		class AWeapon* DamageCauser);

	//** Projectile
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	//** Shotgun
	UFUNCTION(Server, Reliable) // Client���κ��� call�Ǿ� Server���� ����ȴ�.
	void ShotgunServerScoreRequest(
		const TArray<ABaseCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package); // �����ε�
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	FFramePackage GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime);

	//** Hitscan
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package, 
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation);

	//** Projectile
	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	//** Shotgun
	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);

private:
	UPROPERTY()
	ABaseCharacter* Character;
	UPROPERTY()
	class AMainPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.0f;

};