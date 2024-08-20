#include "actor.h"
#include "d_player.h"
#include "p_local.h"
#include "vm.h"
#include "c_cvars.h"
#include "p_trace.h"
#include "p_linetracedata.h"
#include "p_effect.h"

#include "g_levellocals.h"


CVAR(Bool, AimAssistDebugTrace, false, CVAR_ARCHIVE);

constexpr DAngle MAX_ANGLE = DAngle::fromDeg(8.0);
constexpr DAngle RADIAL_PRECISION = DAngle::fromDeg(30.0);
constexpr double MAX_DISTANCE = 5000.0;

constexpr bool debug_traces = true;

DVector3 LevelVec3Diff(FLevelLocals* Level, DVector3 x, DVector3 y);


int AimAssistTrace(AActor* t1, DAngle angle, double distance,
	DAngle pitch, int flags, double sz, FLineTraceData* outdata);

static void AimAssistDoTrace(AActor* self, DAngle i_angle, DAngle i_rotation, AActor*& closest, double& closest_distance, DVector3& hitloc) {
	FLineTraceData t;
	//do a linetrace around i_a and i_r in a circle

	DAngle yaw = self->Angles.Yaw + DAngle::fromDeg(i_rotation.Sin()) * i_angle;
	DAngle pitch = self->Angles.Pitch + DAngle::fromDeg(i_rotation.Cos()) * i_angle;

	if (AimAssistTrace(self,
		yaw,                        //trace angle
		MAX_DISTANCE,               //trace max distance
		pitch,                      //trace pitch
		TRF_NOSKY,                  //trace flags
		self->player->viewheight,   //trace height
		&t                          //output struct
	))
	{
		if (t.HitType == TRACE_HitActor)
		{ //if hit is an actor
			if (!(t.HitActor->flags & (MF_FRIENDLY | MF_CORPSE)))
			{ // if hit is a live monster and not friendly
				double dist = self->Distance3D(t.HitActor);
				if (!closest || dist > closest_distance)
				{ //if it's closer than last hit
				  //change this as new closest
					closest = t.HitActor;
					closest_distance = dist;
					hitloc = t.HitLocation;
				}
				if (AimAssistDebugTrace)
				{
					double hitDist = LevelVec3Diff(&level, self->Pos(), t.HitLocation).Length();
					P_SpawnParticle(
						&level,
						t.HitLocation,
						DVector3(0, 0, 0),
						DVector3(0, 0, 0),
						PalEntry(0xFF, 0x00, 0x00),
						1.0,
						1,
						clamp(hitDist / 100.0, 2.5, 75.0),
						0,
						0,
						1 // SPF_FULLBRIGHT
					);
				}
			}
			else if (AimAssistDebugTrace)
			{
				double hitDist = LevelVec3Diff(&level, self->Pos(), t.HitLocation).Length();
				P_SpawnParticle(
					&level,
					t.HitLocation,
					DVector3(0, 0, 0),
					DVector3(0, 0, 0),
					PalEntry(0xFF, 0xFF, 0x00),
					1.0,
					1,
					clamp(hitDist / 100.0, 2.5, 75.0),
					0,
					0,
					1 // SPF_FULLBRIGHT
				);
			}
		}
		else if (AimAssistDebugTrace)
		{
			double hitDist = LevelVec3Diff(&level, self->Pos(), t.HitLocation).Length();
			P_SpawnParticle(
				&level,
				t.HitLocation,
				DVector3(0, 0, 0),
				DVector3(0, 0, 0),
				PalEntry(0x00, 0xFF, 0x00),
				1.0,
				1,
				clamp(hitDist / 100.0, 2.5, 75.0),
				0,
				0,
				1 // SPF_FULLBRIGHT
			);
		}
	}
}

static TVector2<DAngle> lookAt(DVector3 p1, DVector3 p2) {
	DVector3 delta = LevelVec3Diff(&level, p1, p2);
	return { delta.Angle(), delta.Pitch() };
}

static void RunAimAssist(AActor* self)
{
	assert(self->player);

	if (!self->player->userinfo.GetAimAssistEnabled())
		return;

	DAngle rot_speed = DAngle::fromDeg(self->player->userinfo.GetAimAssistStrength());
	DAngle precision = DAngle::fromDeg(self->player->userinfo.GetAimAssistPrecision());

	double closest_distance = MAX_DISTANCE + 1;
	AActor* closest = nullptr;
	AActor* hit = nullptr;
	DVector3 hitloc(0, 0, 0);

	AimAssistDoTrace(self, nullAngle, nullAngle, closest, closest_distance, hitloc);

	if (!closest) for (DAngle i_a = precision; i_a <= MAX_ANGLE; i_a += precision)
	{
		for (DAngle i_r = nullAngle; !(closest) && i_r <= DAngle360; i_r += RADIAL_PRECISION)
		{
			AimAssistDoTrace(self, i_a, i_r, closest, closest_distance, hitloc);
		}
	}

	if (closest)
	{
		double pheight = self->player->viewheight;
		double aimheight = closest->Height * 0.6;

		DVector3 view = self->Pos() + DVector3(0.0, 0.0, pheight);
		DVector3 target = closest->Pos() + DVector3(0.0, 0.0, aimheight);

		//TVector2<DAngle> target_angle = lookAt(self->Pos(),closest->Pos());
		TVector2<DAngle> target_angle = lookAt(view, target);

		FLineTraceData t;

		AimAssistTrace(self, target_angle.X, MAX_DISTANCE, target_angle.Y, TRF_NOSKY, self->player->viewheight, &t);
		if (t.HitType != TRACE_HitActor || t.HitActor != closest)
		{ //try to aim at correct z

			target_angle = lookAt(view, DVector3(hitloc.X, hitloc.Y, target.Z));

			AimAssistTrace(self, target_angle.X, MAX_DISTANCE, target_angle.Y, TRF_NOSKY, self->player->viewheight, &t);
			if (t.HitType != TRACE_HitActor || t.HitActor != closest)
			{ //try to aim at correct xy

				target_angle = lookAt(view, DVector3(target.X, target.Y, hitloc.Z));

				AimAssistTrace(self, target_angle.X, MAX_DISTANCE, target_angle.Y, TRF_NOSKY, self->player->viewheight, &t);
				if (t.HitType != TRACE_HitActor || t.HitActor != closest)
				{ // aim at trace location
					target_angle = lookAt(view, hitloc);
				}
			}
		}

		TVector2<DAngle> angle_diff(
			deltaangle(self->Angles.Yaw, target_angle.X),
			deltaangle(self->Angles.Pitch, target_angle.Y)
		);

		self->SetAngle((abs(angle_diff.X) <= rot_speed) ? target_angle.X : self->Angles.Yaw + (angle_diff.X.Degrees() > 0.0 ? rot_speed : -rot_speed), SPF_INTERPOLATE);
		self->SetPitch((abs(angle_diff.Y) <= rot_speed) ? target_angle.Y : self->Angles.Pitch + (angle_diff.Y.Degrees() > 0.0 ? rot_speed : -rot_speed), SPF_INTERPOLATE);
	}
}

DEFINE_ACTION_FUNCTION_NATIVE(APlayerPawn, RunAimAssist, RunAimAssist)
{
	PARAM_SELF_PROLOGUE(AActor);
	RunAimAssist(self);
	return 0;
}

CVAR(Bool, AimAssistEnabled, false, CVAR_USERINFO | CVAR_ARCHIVE);
CVAR(Float, AimAssistStrength, 1.0f, CVAR_USERINFO | CVAR_ARCHIVE);
CVAR(Float, AimAssistPrecision, 0.5f, CVAR_USERINFO | CVAR_ARCHIVE);