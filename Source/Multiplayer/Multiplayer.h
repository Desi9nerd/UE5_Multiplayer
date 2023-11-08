#pragma once

#include "CoreMinimal.h"

// Project Settings - Collision: Object Channels에 생성한 SkeletalMesh를 #define함.
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2