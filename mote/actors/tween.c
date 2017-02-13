#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/tween.h>
#include <nemoease.h>

int nemomote_tween_update(struct nemomote *mote, uint32_t type, double secs, struct nemoease *ease, uint32_t dtype, uint32_t tween)
{
	double t;
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_TWEEN_RT(mote, i) -= secs;

		t = nemoease_get(ease,
				NEMOMOTE_TWEEN_DT(mote, i) - NEMOMOTE_TWEEN_RT(mote, i),
				NEMOMOTE_TWEEN_DT(mote, i));

		if (NEMOMOTE_TWEEN_RT(mote, i) <= 0.0f)
			mote->types[i] = dtype;

		if (tween & NEMOMOTE_POSITION_TWEEN) {
			NEMOMOTE_POSITION_X(mote, i) = (NEMOMOTE_TWEEN_DX(mote, i) - NEMOMOTE_TWEEN_SX(mote, i)) * t + NEMOMOTE_TWEEN_SX(mote, i);
			NEMOMOTE_POSITION_Y(mote, i) = (NEMOMOTE_TWEEN_DY(mote, i) - NEMOMOTE_TWEEN_SY(mote, i)) * t + NEMOMOTE_TWEEN_SY(mote, i);
		}

		if (tween & NEMOMOTE_COLOR_TWEEN) {
			NEMOMOTE_COLOR_R(mote, i) = (NEMOMOTE_TWEEN_DR(mote, i) - NEMOMOTE_TWEEN_SR(mote, i)) * t + NEMOMOTE_TWEEN_SR(mote, i);
			NEMOMOTE_COLOR_G(mote, i) = (NEMOMOTE_TWEEN_DG(mote, i) - NEMOMOTE_TWEEN_SG(mote, i)) * t + NEMOMOTE_TWEEN_SG(mote, i);
			NEMOMOTE_COLOR_B(mote, i) = (NEMOMOTE_TWEEN_DB(mote, i) - NEMOMOTE_TWEEN_SB(mote, i)) * t + NEMOMOTE_TWEEN_SB(mote, i);
			NEMOMOTE_COLOR_A(mote, i) = (NEMOMOTE_TWEEN_DA(mote, i) - NEMOMOTE_TWEEN_SA(mote, i)) * t + NEMOMOTE_TWEEN_SA(mote, i);
		}

		if (tween & NEMOMOTE_MASS_TWEEN) {
			NEMOMOTE_MASS(mote, i) = (NEMOMOTE_TWEEN_DM(mote, i) - NEMOMOTE_TWEEN_SM(mote, i)) * t + NEMOMOTE_TWEEN_SM(mote, i);
		}
	}

	return 0;
}
