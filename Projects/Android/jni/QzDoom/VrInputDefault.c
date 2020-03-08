/************************************************************************************

Filename	:	VrInputDefault.c
Content		:	Handles default controller input
Created		:	August 2019
Authors		:	Simon Brown

*************************************************************************************/

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_SystemUtils.h>
#include <VrApi_Input.h>
#include <VrApi_Types.h>

#include "VrInput.h"

#include "doomkeys.h"

int getGameState();
int isMenuActive();
void Joy_GenerateButtonEvents(int oldbuttons, int newbuttons, int numbuttons, int base);


void HandleInput_Default( ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTracking* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTracking* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 )

{

    static bool dominantGripPushed = false;

    //Show screen view (if in multiplayer toggle scoreboard)
    if (((pOffTrackedRemoteNew->Buttons & offButton2) !=
         (pOffTrackedRemoteOld->Buttons & offButton2)) &&
			(pOffTrackedRemoteNew->Buttons & offButton2)) {


    }

	//Menu button
	handleTrackedControllerButton(&leftTrackedRemoteState_new, &leftTrackedRemoteState_old, ovrButton_Enter, KEY_ESCAPE);

    if (getGameState() != 0 || isMenuActive()) //gamestate != GS_LEVEL
    {
        Joy_GenerateButtonEvents((pOffTrackedRemoteOld->Joystick.x > 0.7f ? 1 : 0), (pOffTrackedRemoteNew->Joystick.x > 0.7f ? 1 : 0), 1, KEY_PAD_DPAD_RIGHT);

        Joy_GenerateButtonEvents((pOffTrackedRemoteOld->Joystick.x < -0.7f ? 1 : 0), (pOffTrackedRemoteNew->Joystick.x < -0.7f ? 1 : 0), 1, KEY_PAD_DPAD_LEFT);

        Joy_GenerateButtonEvents((pOffTrackedRemoteOld->Joystick.y < -0.7f ? 1 : 0), (pOffTrackedRemoteNew->Joystick.y < -0.7f ? 1 : 0), 1, KEY_PAD_DPAD_DOWN);

        Joy_GenerateButtonEvents((pOffTrackedRemoteOld->Joystick.y > 0.7f ? 1 : 0), (pOffTrackedRemoteNew->Joystick.y > 0.7f ? 1 : 0), 1, KEY_PAD_DPAD_UP);

        handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, domButton1, KEY_PAD_A);
        handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, ovrButton_Trigger, KEY_PAD_A);
        handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, domButton2, KEY_PAD_B);
    }
    else
    {
        //Dominant Grip works like a shift key
        if ((pDominantTrackedRemoteNew->Buttons & ovrButton_GripTrigger) !=
            (pDominantTrackedRemoteOld->Buttons & ovrButton_GripTrigger)) {

            dominantGripPushed = true;
        }

        float distance = sqrtf(powf(pOffTracking->HeadPose.Pose.Position.x - pDominantTracking->HeadPose.Pose.Position.x, 2) +
                               powf(pOffTracking->HeadPose.Pose.Position.y - pDominantTracking->HeadPose.Pose.Position.y, 2) +
                               powf(pOffTracking->HeadPose.Pose.Position.z - pDominantTracking->HeadPose.Pose.Position.z, 2));

        //Turn on weapon stabilisation?
        if ((pOffTrackedRemoteNew->Buttons & ovrButton_GripTrigger) !=
            (pOffTrackedRemoteOld->Buttons & ovrButton_GripTrigger)) {

            if (pOffTrackedRemoteNew->Buttons & ovrButton_GripTrigger)
            {
                if (distance < 0.50f)
                {
                    weaponStabilised = true;
                }
            }
            else
            {
                weaponStabilised = false;
            }
        }

        //dominant hand stuff first
        {
			///Weapon location relative to view
            weaponoffset[0] = pDominantTracking->HeadPose.Pose.Position.x - hmdPosition[0];
            weaponoffset[1] = pDominantTracking->HeadPose.Pose.Position.y - hmdPosition[1];
            weaponoffset[2] = pDominantTracking->HeadPose.Pose.Position.z - hmdPosition[2];

			{
				vec2_t v;
//				rotateAboutOrigin(-weaponoffset[0], weaponoffset[2], (doomYawDegrees - hmdorientation[YAW]), v);
				weaponoffset[0] = v[0];
				weaponoffset[2] = v[1];
			}

            //Set gun angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
            const ovrQuatf quatRemote = pDominantTracking->HeadPose.Pose.Orientation;
            QuatToYawPitchRoll(quatRemote, vr_weapon_pitchadjust, weaponangles);
            weaponangles[YAW] += (doomYawDegrees - hmdorientation[YAW]);
            weaponangles[ROLL] *= -1.0f;


            if (weaponStabilised)
            {
                float z = pOffTracking->HeadPose.Pose.Position.z - pDominantTracking->HeadPose.Pose.Position.z;
                float x = pOffTracking->HeadPose.Pose.Position.x - pDominantTracking->HeadPose.Pose.Position.x;
                float y = pOffTracking->HeadPose.Pose.Position.y - pDominantTracking->HeadPose.Pose.Position.y;
                float zxDist = length(x, z);

                if (zxDist != 0.0f && z != 0.0f) {
                    VectorSet(weaponangles, -degrees(atanf(y / zxDist)), (doomYawDegrees - hmdorientation[YAW]) - degrees(atan2f(x, -z)), weaponangles[ROLL]);
                }
            }
        }

        float controllerYawHeading = 0.0f;
        //off-hand stuff
        {
            offhandoffset[0] = pOffTracking->HeadPose.Pose.Position.x - hmdPosition[0];
            offhandoffset[1] = pOffTracking->HeadPose.Pose.Position.y - hmdPosition[1];
            offhandoffset[2] = pOffTracking->HeadPose.Pose.Position.z - hmdPosition[2];

			vec2_t v;
			rotateAboutOrigin(-offhandoffset[0], offhandoffset[2], (doomYawDegrees - hmdorientation[YAW]), v);
			offhandoffset[0] = v[0];
			offhandoffset[2] = v[1];

            QuatToYawPitchRoll(pOffTracking->HeadPose.Pose.Orientation, 15.0f, offhandangles);

            offhandangles[YAW] += (doomYawDegrees - hmdorientation[YAW]);

			if (vr_walkdirection == 0) {
				controllerYawHeading = -doomYawDegrees + offhandangles[YAW];
			}
			else
			{
				controllerYawHeading = 0.0f;//-cl.viewangles[YAW];
			}
        }

        //Right-hand specific stuff
        {
            ALOGV("        Right-Controller-Position: %f, %f, %f",
                  pDominantTracking->HeadPose.Pose.Position.x,
				  pDominantTracking->HeadPose.Pose.Position.y,
				  pDominantTracking->HeadPose.Pose.Position.z);

            //This section corrects for the fact that the controller actually controls direction of movement, but we want to move relative to the direction the
            //player is facing for positional tracking
            float vr_positional_factor = 1.0f;//4.0f;

            vec2_t v;
            rotateAboutOrigin(-positionDeltaThisFrame[0] * vr_positional_factor,
                              positionDeltaThisFrame[2] * vr_positional_factor, - hmdorientation[YAW], v);
            positional_movementSideways = v[0];
            positional_movementForward = v[1];

            ALOGV("        positional_movementSideways: %f, positional_movementForward: %f",
                  positional_movementSideways,
                  positional_movementForward);

            //Jump (B Button)
            handleTrackedControllerButton(pDominantTrackedRemoteNew,
                                          pDominantTrackedRemoteOld, domButton2, KEY_SPACE);

            //We need to record if we have started firing primary so that releasing trigger will stop firing, if user has pushed grip
            //in meantime, then it wouldn't stop the gun firing and it would get stuck
            static bool firingPrimary = false;

			{
				//Fire Primary
				if ((pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger) !=
					(pDominantTrackedRemoteOld->Buttons & ovrButton_Trigger)) {

					firingPrimary = (pDominantTrackedRemoteNew->Buttons & ovrButton_Trigger);

				}
			}

            //Duck with A
            if ((pDominantTrackedRemoteNew->Buttons & domButton1) !=
                (pDominantTrackedRemoteOld->Buttons & domButton1) &&
                ducked != DUCK_CROUCHED) {

                ducked = (pDominantTrackedRemoteNew->Buttons & domButton1) ? DUCK_BUTTON : DUCK_NOTDUCKED;

                //Trigger Duck
            }

			//Weapon/Inventory Chooser
			static bool itemSwitched = false;
			if (between(-0.2f, pDominantTrackedRemoteNew->Joystick.x, 0.2f) &&
				(between(0.8f, pDominantTrackedRemoteNew->Joystick.y, 1.0f) ||
				 between(-1.0f, pDominantTrackedRemoteNew->Joystick.y, -0.8f)))
			{
				if (!itemSwitched) {
					if (between(0.8f, pDominantTrackedRemoteNew->Joystick.y, 1.0f))
					{
					}
					else
					{
					}
					itemSwitched = true;
				}
			} else {
				itemSwitched = false;
			}
        }

        //Left-hand specific stuff
        {
            ALOGV("        Left-Controller-Position: %f, %f, %f",
                  pOffTracking->HeadPose.Pose.Position.x,
				  pOffTracking->HeadPose.Pose.Position.y,
				  pOffTracking->HeadPose.Pose.Position.z);

			//Laser-sight
			if ((pDominantTrackedRemoteNew->Buttons & ovrButton_Joystick) !=
				(pDominantTrackedRemoteOld->Buttons & ovrButton_Joystick)
				&& (pDominantTrackedRemoteNew->Buttons & ovrButton_Joystick)) {

			}

			//Apply a filter and quadratic scaler so small movements are easier to make
			float dist = length(pOffTrackedRemoteNew->Joystick.x, pOffTrackedRemoteNew->Joystick.y);
			float nlf = nonLinearFilter(dist);
            float x = nlf * pOffTrackedRemoteNew->Joystick.x;
            float y = nlf * pOffTrackedRemoteNew->Joystick.y;

            player_moving = (fabs(x) + fabs(y)) > 0.05f;

			//Adjust to be off-hand controller oriented
            vec2_t v;
            rotateAboutOrigin(x, y, controllerYawHeading, v);

            remote_movementSideways = v[0];
            remote_movementForward = v[1];
            ALOGV("        remote_movementSideways: %f, remote_movementForward: %f",
                  remote_movementSideways,
                  remote_movementForward);


            //show help computer while X/A pressed
            if ((pOffTrackedRemoteNew->Buttons & offButton1) !=
                 (pOffTrackedRemoteOld->Buttons & offButton1)) {

                //Help Computer
            }


            //Use (Action)
            if ((pOffTrackedRemoteNew->Buttons & ovrButton_Joystick) !=
                (pOffTrackedRemoteOld->Buttons & ovrButton_Joystick)
                && (pOffTrackedRemoteNew->Buttons & ovrButton_Joystick)) {


            }

            //We need to record if we have started firing primary so that releasing trigger will stop definitely firing, if user has pushed grip
            //in meantime, then it wouldn't stop the gun firing and it would get stuck
            static bool firingPrimary = false;

			//Run
//			handleTrackedControllerButton(pOffTrackedRemoteNew,
//										  pOffTrackedRemoteOld,
//										  ovrButton_Trigger, K_SHIFT);

            static int increaseSnap = true;
			if (pDominantTrackedRemoteNew->Joystick.x > 0.6f)
			{
				if (increaseSnap)
				{
					snapTurn -= vr_snapturn_angle;
                    if (vr_snapturn_angle > 10.0f) {
                        increaseSnap = false;
                    }

                    if (snapTurn < -180.0f)
                    {
                        snapTurn += 360.f;
                    }
                }
			} else if (pDominantTrackedRemoteNew->Joystick.x < 0.4f) {
				increaseSnap = true;
			}

			static int decreaseSnap = true;
			if (pDominantTrackedRemoteNew->Joystick.x < -0.6f)
			{
				if (decreaseSnap)
				{
					snapTurn += vr_snapturn_angle;

					//If snap turn configured for less than 10 degrees
					if (vr_snapturn_angle > 10.0f) {
                        decreaseSnap = false;
                    }

                    if (snapTurn > 180.0f)
                    {
                        snapTurn -= 360.f;
                    }
				}
			} else if (pDominantTrackedRemoteNew->Joystick.x > -0.4f)
			{
				decreaseSnap = true;
			}
        }
    }

    //Save state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}