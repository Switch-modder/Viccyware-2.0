/**
* File: behaviorCoordinateGlobalInterrupts.cpp
*
* Author: Kevin M. Karol
* Created: 2/22/18
*
* Description: Behavior responsible for handling special case needs
* that require coordination across behavior global interrupts
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateGlobalInterrupts.h"

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorDriveToFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/photographyManager.h"

#include "util/helpers/boundedWhile.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/types/behaviorComponent/streamAndLightEffect.h"

#include <deque>

namespace Anki {
namespace Vector {

namespace{

  // add behavior _classes_ here if we should disable the prox-based "react to sudden obstacle" behavior while
  // _any_ behavior of that class is running below us on the stack
  static const std::set<BehaviorClass> kBehaviorClassesToSuppressProx = {{ BEHAVIOR_CLASS(BlackJack),
                                                                           BEHAVIOR_CLASS(FistBump),
                                                                           BEHAVIOR_CLASS(FindCube),
                                                                           BEHAVIOR_CLASS(Keepaway),
                                                                           BEHAVIOR_CLASS(InspectCube),
                                                                           BEHAVIOR_CLASS(PickUpCube),
                                                                           BEHAVIOR_CLASS(PopAWheelie),
                                                                           BEHAVIOR_CLASS(PounceWithProx),
                                                                           BEHAVIOR_CLASS(RollBlock),
                                                                           BEHAVIOR_CLASS(PossiblePerformance) }};

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressReactToSound = {{ BEHAVIOR_CLASS(BlackJack),
                                                                                   BEHAVIOR_CLASS(DanceToTheBeat),
                                                                                   BEHAVIOR_CLASS(FetchCube),
                                                                                   BEHAVIOR_CLASS(FistBump),
                                                                                   BEHAVIOR_CLASS(Keepaway),
                                                                                   BEHAVIOR_CLASS(ListenForBeats),
                                                                                   BEHAVIOR_CLASS(InspectCube),
                                                                                   BEHAVIOR_CLASS(PickUpCube),
                                                                                   BEHAVIOR_CLASS(PopAWheelie),
                                                                                   BEHAVIOR_CLASS(PounceWithProx),
                                                                                   BEHAVIOR_CLASS(RollBlock),
                                                                                   BEHAVIOR_CLASS(FindCubeAndThen),
                                                                                   BEHAVIOR_CLASS(PossiblePerformance)}};

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressTouch = {{ BEHAVIOR_CLASS(BlackJack),
                                                                            BEHAVIOR_CLASS(PossiblePerformance) }};

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressCliff = {{ BEHAVIOR_CLASS(BlackJack),
                                                                            BEHAVIOR_CLASS(FetchCube) }};

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressTimerAntics = {{ BEHAVIOR_CLASS(BlackJack),
                                                                                  BEHAVIOR_CLASS(CoordinateWeather),
                                                                                  BEHAVIOR_CLASS(PossiblePerformance) }};

  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenMeetVictor = {{
    BEHAVIOR_ID(ReactToTouchPetting),       // the user will often turn the robot to face them and in the process touch it
    BEHAVIOR_ID(ReactToUnexpectedMovement), // the user will often turn the robot to face them
    BEHAVIOR_ID(ReactToSoundAwake),         // fully concentrate on what's in front
    BEHAVIOR_ID(ReactToDarkness)            // user hand near camera may trigger darkened condition
  }};
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenDancingToTheBeat = {
    BEHAVIOR_ID(ReactToObstacle),
    BEHAVIOR_ID(ReactToSoundAwake),
  };
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenGoingHome = {
    BEHAVIOR_ID(DanceToTheBeatCoordinator),
    BEHAVIOR_ID(ListenForBeats),
    BEHAVIOR_ID(DanceToTheBeat),
    BEHAVIOR_ID(ReactToObstacle),
  };

  static const std::set<UserIntentTag> kUserIntentTagsToSuppressWakeWordTurn = {{
    USER_INTENT(imperative_findcube),
    USER_INTENT(system_charger),
    USER_INTENT(movement_backward),
    USER_INTENT(movement_turnleft),
    USER_INTENT(movement_turnright),
    USER_INTENT(movement_turnaround),
  }};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::DynamicVariables::DynamicVariables()
  : suppressProx(false)
{
}


///////////
/// BehaviorCoordinateGlobalInterrupts
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::BehaviorCoordinateGlobalInterrupts(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::~BehaviorCoordinateGlobalInterrupts()
{

}


void BehaviorCoordinateGlobalInterrupts::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.wakeWordBehavior         = BC.FindBehaviorByID(BEHAVIOR_ID(TriggerWordDetected));

  for( const auto& id : kBehaviorIDsToSuppressWhenMeetVictor ) {
    _iConfig.toSuppressWhenMeetVictor.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenDancingToTheBeat ) {
    _iConfig.toSuppressWhenDancingToTheBeat.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenGoingHome ) {
    _iConfig.toSuppressWhenGoingHome.push_back( BC.FindBehaviorByID(id) );
  }

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TimerUtilityCoordinator),
                                 BEHAVIOR_CLASS(TimerUtilityCoordinator),
                                 _iConfig.timerCoordBehavior);
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TriggerWordDetected),
                                 BEHAVIOR_CLASS(ReactToVoiceCommand),
                                 _iConfig.reactToVoiceCommandBehavior);

  _iConfig.triggerWordPendingCond = BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel());
  _iConfig.triggerWordPendingCond->Init(GetBEI());

  _iConfig.reactToObstacleBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToObstacle));
  _iConfig.meetVictorBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(MeetVictor));
  _iConfig.danceToTheBeatBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DanceToTheBeat));

  _iConfig.behaviorsThatShouldntReactToUnexpectedMovement.AddBehavior(BC, BEHAVIOR_CLASS(BumpObject));
  _iConfig.behaviorsThatShouldntReactToUnexpectedMovement.AddBehavior(BC, BEHAVIOR_CLASS(ClearChargerArea));
  _iConfig.behaviorsThatShouldntReactToUnexpectedMovement.AddBehavior(BC, BEHAVIOR_CLASS(ReactToHand));
  _iConfig.reactToUnexpectedMovementBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToUnexpectedMovement));

  _iConfig.reactToSoundAwakeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToSoundAwake));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressReactToSound){
    _iConfig.behaviorsThatShouldntReactToSoundAwake.AddBehavior(BC, behaviorClass);
  }

  _iConfig.reactToTouchPettingBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToTouchPetting));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressTouch){
    _iConfig.behaviorsThatShouldntReactToTouch.AddBehavior(BC, behaviorClass);
  }

  for(const auto& behaviorClass : kBehaviorClassesToSuppressTimerAntics){
    _iConfig.behaviorsThatShouldSuppressTimerAntics.AddBehavior(BC, behaviorClass);
  }

  _iConfig.reactToCliffBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToCliff));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressCliff){
    _iConfig.behaviorsThatShouldntReactToCliff.AddBehavior(BC, behaviorClass);
  }

  std::set<ICozmoBehaviorPtr> driveToFaceBehaviors = BC.FindBehaviorsByClass(BEHAVIOR_CLASS(DriveToFace));
  _iConfig.driveToFaceBehaviors.reserve( driveToFaceBehaviors.size() );
  for( const auto& ptr : driveToFaceBehaviors ) {
    auto beh = std::dynamic_pointer_cast<BehaviorDriveToFace>(ptr);
    if( beh != nullptr ) {
      _iConfig.driveToFaceBehaviors.push_back( beh );
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCoordinateGlobalInterrupts::WantsToBeActivatedBehavior() const
{
  // always wants to be activated
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnBehaviorActivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  
  robotInfo.StartDoom();
  
  robotInfo.GetMoveComponent().EnableLiftPower(false);
  robotInfo.GetMoveComponent().EnableHeadPower(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::CreateConsoleVars()
{
  // deque can contain non-copyable objects. its kept here to keep the header cleaner
  static std::deque<Anki::Util::ConsoleVar<bool>> vars;
  if( !vars.empty() ) {
    return;
  }
  const auto& BC = GetBEI().GetBehaviorContainer();
  std::set<IBehavior*> passThroughList;
  GetLinkedActivatableScopeBehaviors( passThroughList );
  if( !passThroughList.empty() ) {
    std::set<IBehavior*> globalInterruptions;
    (*passThroughList.begin())->GetAllDelegates( globalInterruptions );
    for( const auto* delegate : globalInterruptions ) {
      const auto* cozmoDelegate = dynamic_cast<const ICozmoBehavior*>( delegate );
      if( cozmoDelegate != nullptr ) {
        BehaviorID id = cozmoDelegate->GetID();
        auto pairIt = _iConfig.devActivatableOverrides.emplace( BC.FindBehaviorByID(id), true );
        std::string name = std::string{"Toggle_"} + BehaviorTypesWrapper::BehaviorIDToString( id );
        vars.emplace_back( pairIt.first->second,
                           name.c_str(),
                           "BehaviorCoordinateGlobalInterrupts",
                           true );
      }
    }
  }
}



} // namespace Vector
} // namespace Anki
