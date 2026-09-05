#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct BARUGAME_API FBaruGameplayTags
{
public:
	static const FBaruGameplayTags& Get()
	{
		if (!bIsInitialized)
		{
			InitializeNativeGameplayTags();
		}
		return GameplayTags;
	}
	
	static void InitializeNativeGameplayTags();

	// =========================================================================
	// Input Tags
	// =========================================================================
	FGameplayTag InputTag_Move;
	FGameplayTag InputTag_Look;
	FGameplayTag InputTag_Sprint;
	FGameplayTag InputTag_Interact;
	FGameplayTag InputTag_UseItem;
	FGameplayTag InputTag_DropItem;
	FGameplayTag InputTag_Ping;
	FGameplayTag InputTag_Ability_Primary;
	FGameplayTag InputTag_Ability_Secondary;
	FGameplayTag InputTag_Reload;

	// =========================================================================
	// State & Status Tags
	// =========================================================================
	FGameplayTag State_Dead;
	FGameplayTag State_DBNO;
	
	FGameplayTag State_Sanity_Stage1;
	FGameplayTag State_Sanity_Stage2;
	FGameplayTag State_Sanity_Stage3;
	FGameplayTag State_Sanity_Frenzy;
	
	FGameplayTag State_Extracting;
	FGameplayTag State_Extracted;
	
	FGameplayTag State_Immune;
	
	FGameplayTag State_Combat_Reloading;
	FGameplayTag State_Combat_Aiming;
	
	FGameplayTag State_Debuff_Groggy;

	// =========================================================================
	// Combat & Damage Tags
	// =========================================================================
	// SetByCaller Magnitude Keys
	FGameplayTag Data_Damage;
	FGameplayTag Data_Damage_Special;
	FGameplayTag Data_Damage_Suppression;

	// Damage Types
	FGameplayTag Damage_Type_Physical;
	FGameplayTag Damage_Type_Sanity;
	FGameplayTag Damage_Type_Special;

	// Hit Reactions
	FGameplayTag Damage_HitReaction_Light;
	FGameplayTag Damage_HitReaction_Heavy;

	// =========================================================================
	// Interaction Tags
	// =========================================================================
	FGameplayTag Interaction_Type_Pickup;
	FGameplayTag Interaction_Type_Door;
	FGameplayTag Interaction_Type_CoOp;
	FGameplayTag Interaction_Type_Elevator;
	FGameplayTag Interaction_Type_Fuse;

	// =========================================================================
	// Event & Directing Tags
	// =========================================================================
	FGameplayTag Event_Montage_Hit;
	FGameplayTag Event_Montage_End;
	FGameplayTag Event_Noise_Footstep;
	FGameplayTag Event_Noise_Loud;

	// =========================================================================
	// Ability & Failure Tags
	// =========================================================================
	FGameplayTag Ability_Action_Reload;
	FGameplayTag Ability_ActivateFail_Cooldown;
	FGameplayTag Ability_ActivateFail_Cost;
	FGameplayTag Ability_ActivateFail_TagsBlocked;
	
	// =========================================================================
	// Monster Ability Tags
	// =========================================================================
	FGameplayTag Ability_Action_Monster_Attack;
	
	// =========================================================================
	// Weapon Classification Tags
	// =========================================================================
	FGameplayTag Weapon_Type_Rifle;
	FGameplayTag Weapon_Type_Revolver;
	FGameplayTag Weapon_Type_Grenade;

	// =========================================================================
	// Weapon Action Tags
	// =========================================================================
	FGameplayTag Ability_Action_Fire_Rifle;
	FGameplayTag Ability_Action_Fire_Revolver;
	FGameplayTag Ability_Action_Throw_Grenade;
	FGameplayTag Ability_Action_Reload_Rifle;
	FGameplayTag Ability_Action_Reload_Revolver;

	// =========================================================================
	// Damage & Feedback (Cue) Tags
	// =========================================================================
	// Damage Type
	FGameplayTag Damage_Type_Explosion;

	// GameplayCue Types
	FGameplayTag GameplayCue_Weapon_Rifle_Fire;
	FGameplayTag GameplayCue_Weapon_Revolver_Fire;
	FGameplayTag GameplayCue_Weapon_Grenade_Explode;

private:
	static FBaruGameplayTags GameplayTags;
	static bool bIsInitialized;
};