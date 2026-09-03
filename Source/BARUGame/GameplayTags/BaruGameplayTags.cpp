#include "GameplayTags/BaruGameplayTags.h"
#include "GameplayTagsManager.h"
#include "BaruLog.h"

FBaruGameplayTags FBaruGameplayTags::GameplayTags;

void FBaruGameplayTags::InitializeNativeGameplayTags()
{
    UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

    // =========================================================================
	// Input Tags
	// =========================================================================
	GameplayTags.InputTag_Move = Manager.AddNativeGameplayTag(TEXT("InputTag.Move"), TEXT("WASD Movement"));
	GameplayTags.InputTag_Look = Manager.AddNativeGameplayTag(TEXT("InputTag.Look"), TEXT("Mouse Look"));
	GameplayTags.InputTag_Sprint = Manager.AddNativeGameplayTag(TEXT("InputTag.Sprint"), TEXT("Sprint Action"));
	GameplayTags.InputTag_Interact = Manager.AddNativeGameplayTag(TEXT("InputTag.Interact"), TEXT("Interact Action"));
	GameplayTags.InputTag_UseItem = Manager.AddNativeGameplayTag(TEXT("InputTag.UseItem"), TEXT("Use Item Action"));
	GameplayTags.InputTag_DropItem = Manager.AddNativeGameplayTag(TEXT("InputTag.DropItem"), TEXT("Drop Item Action"));
	GameplayTags.InputTag_Ping = Manager.AddNativeGameplayTag(TEXT("InputTag.Ping"), TEXT("Send Ping Action"));
	GameplayTags.InputTag_Ability_Primary = Manager.AddNativeGameplayTag(TEXT("InputTag.Ability.Primary"), TEXT("Primary Attack/Skill"));
	GameplayTags.InputTag_Ability_Secondary = Manager.AddNativeGameplayTag(TEXT("InputTag.Ability.Secondary"), TEXT("Secondary Attack/Skill"));
	GameplayTags.InputTag_Reload = Manager.AddNativeGameplayTag(TEXT("InputTag.Reload"), TEXT("Reload Weapon Action"));

	// =========================================================================
	// State & Status Tags
	// =========================================================================
	GameplayTags.State_Dead = Manager.AddNativeGameplayTag(TEXT("State.Dead"), TEXT("Actor is Dead"));
	GameplayTags.State_DBNO = Manager.AddNativeGameplayTag(TEXT("State.DBNO"), TEXT("Down But Not Out"));
	GameplayTags.State_Sanity_Stage1 = Manager.AddNativeGameplayTag(TEXT("State.Sanity.Stage1"), TEXT("Sanity 100-70"));
	GameplayTags.State_Sanity_Stage2 = Manager.AddNativeGameplayTag(TEXT("State.Sanity.Stage2"), TEXT("Sanity 70-40"));
	GameplayTags.State_Sanity_Stage3 = Manager.AddNativeGameplayTag(TEXT("State.Sanity.Stage3"), TEXT("Sanity 40-10"));
	GameplayTags.State_Sanity_Frenzy = Manager.AddNativeGameplayTag(TEXT("State.Sanity.Frenzy"), TEXT("Sanity 10-0 Frenzy"));
	GameplayTags.State_Extracting = Manager.AddNativeGameplayTag(TEXT("State.Extracting"), TEXT("Extracting in Progress"));
	GameplayTags.State_Extracted = Manager.AddNativeGameplayTag(TEXT("State.Extracted"), TEXT("Extraction Successful"));
	GameplayTags.State_Immune = Manager.AddNativeGameplayTag(TEXT("State.Immune"), TEXT("Invulnerable State"));
	GameplayTags.State_Combat_Reloading = Manager.AddNativeGameplayTag(TEXT("State.Combat.Reloading"), TEXT("Actor is Reloading Weapon"));
	
	// [추가] 몬스터 제압 및 디버프 상태
	GameplayTags.State_Debuff_Groggy = Manager.AddNativeGameplayTag(TEXT("State.Debuff.Groggy"), TEXT("Monster is in Groggy State"));

	// =========================================================================
	// Combat & Damage Tags
	// =========================================================================
	GameplayTags.Data_Damage = Manager.AddNativeGameplayTag(TEXT("Data.Damage"), TEXT("SetByCaller Physical Damage Magnitude Key"));
	GameplayTags.Data_Damage_Special = Manager.AddNativeGameplayTag(TEXT("Data.Damage.Special"), TEXT("SetByCaller Special Damage Magnitude Key"));
	GameplayTags.Data_Damage_Suppression = Manager.AddNativeGameplayTag(TEXT("Data.Damage.Suppression"), TEXT("SetByCaller Suppression Damage Magnitude Key"));

	GameplayTags.Damage_Type_Physical = Manager.AddNativeGameplayTag(TEXT("Damage.Type.Physical"), TEXT("Physical Damage Type"));
	GameplayTags.Damage_Type_Sanity = Manager.AddNativeGameplayTag(TEXT("Damage.Type.Sanity"), TEXT("Sanity Damage Type"));
	GameplayTags.Damage_Type_Special = Manager.AddNativeGameplayTag(TEXT("Damage.Type.Special"), TEXT("Special Damage Type"));

	GameplayTags.Damage_HitReaction_Light = Manager.AddNativeGameplayTag(TEXT("Damage.HitReaction.Light"), TEXT("Light Hit Reaction"));
	GameplayTags.Damage_HitReaction_Heavy = Manager.AddNativeGameplayTag(TEXT("Damage.HitReaction.Heavy"), TEXT("Heavy Hit Reaction"));

	// =========================================================================
	// Interaction Tags
	// =========================================================================
	GameplayTags.Interaction_Type_Pickup = Manager.AddNativeGameplayTag(TEXT("Interaction.Type.Pickup"), TEXT("Pickup Item"));
	GameplayTags.Interaction_Type_Door = Manager.AddNativeGameplayTag(TEXT("Interaction.Type.Door"), TEXT("Open/Close Door"));
	GameplayTags.Interaction_Type_CoOp = Manager.AddNativeGameplayTag(TEXT("Interaction.Type.CoOp"), TEXT("Cooperative Interaction"));
	GameplayTags.Interaction_Type_Elevator = Manager.AddNativeGameplayTag(TEXT("Interaction.Type.Elevator"), TEXT("Elevator Control"));
	GameplayTags.Interaction_Type_Fuse = Manager.AddNativeGameplayTag(TEXT("Interaction.Type.Fuse"), TEXT("Fuse Box Insert"));

	// =========================================================================
	// Event & Directing Tags
	// =========================================================================
	GameplayTags.Event_Montage_Hit = Manager.AddNativeGameplayTag(TEXT("Event.Montage.Hit"), TEXT("Montage Hit Frame"));
	GameplayTags.Event_Montage_End = Manager.AddNativeGameplayTag(TEXT("Event.Montage.End"), TEXT("Montage Ended"));
	GameplayTags.Event_Noise_Footstep = Manager.AddNativeGameplayTag(TEXT("Event.Noise.Footstep"), TEXT("Footstep Noise"));
	GameplayTags.Event_Noise_Loud = Manager.AddNativeGameplayTag(TEXT("Event.Noise.Loud"), TEXT("Gunfire or Loud Noise"));

	// =========================================================================
	// Ability & Failure Tags
	// =========================================================================
	GameplayTags.Ability_Action_Reload = Manager.AddNativeGameplayTag(TEXT("Ability.Action.Reload"), TEXT("Reload Ability Action"));
	
	GameplayTags.Ability_ActivateFail_Cooldown = Manager.AddNativeGameplayTag(TEXT("Ability.ActivateFail.Cooldown"), TEXT("Ability Activation Failed: Cooldown"));
	GameplayTags.Ability_ActivateFail_Cost = Manager.AddNativeGameplayTag(TEXT("Ability.ActivateFail.Cost"), TEXT("Ability Activation Failed: Insufficient Cost"));
	GameplayTags.Ability_ActivateFail_TagsBlocked = Manager.AddNativeGameplayTag(TEXT("Ability.ActivateFail.TagsBlocked"), TEXT("Ability Activation Failed: Tags Blocked"));

	BARU_LOG(LogBaruGAS, Log, TEXT("FBaruGameplayTags initialized with Extended Native Tags."));
}