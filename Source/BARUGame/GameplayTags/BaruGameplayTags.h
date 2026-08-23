#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct BARUGAME_API FBaruGameplayTags
{
public:
	static const FBaruGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	// Input Tags
	FGameplayTag InputTag_Move;
	FGameplayTag InputTag_Look;
	FGameplayTag InputTag_Sprint;
	FGameplayTag InputTag_Interact;
	FGameplayTag InputTag_UseItem;
	FGameplayTag InputTag_DropItem;
	FGameplayTag InputTag_Ping;
	FGameplayTag InputTag_Ability_Primary;
	FGameplayTag InputTag_Ability_Secondary;

	// State & Status Tags
	FGameplayTag State_Dead;
	FGameplayTag State_DBNO;
	FGameplayTag State_Sanity_Stage1;
	FGameplayTag State_Sanity_Stage2;
	FGameplayTag State_Sanity_Stage3;
	FGameplayTag State_Sanity_Frenzy;
	FGameplayTag State_Extracting;
	FGameplayTag State_Extracted;
	FGameplayTag State_Immune;

	// Combat & Damage Tags
	FGameplayTag Damage_Type_Physical;
	FGameplayTag Damage_Type_Sanity;
	FGameplayTag Damage_HitReaction_Light;
	FGameplayTag Damage_HitReaction_Heavy;

	// Interaction Tags
	FGameplayTag Interaction_Type_Pickup;
	FGameplayTag Interaction_Type_Door;
	FGameplayTag Interaction_Type_CoOp;
	FGameplayTag Interaction_Type_Elevator;
	FGameplayTag Interaction_Type_Fuse;

	// Event & Directing Tags
	FGameplayTag Event_Montage_Hit;
	FGameplayTag Event_Montage_End;
	FGameplayTag Event_Noise_Footstep;
	FGameplayTag Event_Noise_Loud;

private:
	static FBaruGameplayTags GameplayTags;
};