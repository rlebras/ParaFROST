/***********************************************************************[backtrack.cpp]
Copyright(c) 2020, Muhammad Osama - Anton Wijs,
Technische Universiteit Eindhoven (TU/e).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
**********************************************************************************/

#include "solve.h"
using namespace pFROST;

inline void ParaFROST::savePhases() {

	const LIT_ST reset = (last.rephase.type && stats.conflicts > last.rephase.conflicts);
	if (!probed) {
		if (reset) last.rephase.target = 0;
		if (sp->trailpivot > last.rephase.target) {
			last.rephase.target = sp->trailpivot;
			savePhases(sp->ptarget);
		}
		if (sp->trailpivot > last.rephase.best) {
			last.rephase.best = sp->trailpivot;
			savePhases(sp->pbest);
		}
		sp->trailpivot = 0;
	}
	if (reset) last.rephase.type = 0;
}

inline void	ParaFROST::cancelAssign(const uint32& lit) {
	CHECKLIT(lit);
	assert(inf.unassigned < inf.maxVar);
	sp->value[lit] = UNDEFINED;
	sp->value[FLIP(lit)] = UNDEFINED;
	inf.unassigned++;
	PFLOG2(4, "  literal %d@%d cancelled", l2i(lit), l2dl(lit));
}

void ParaFROST::backtrack(const int& jmplevel)
{
	if (DL() == jmplevel) return;
	const int pivot = jmplevel + 1;
	PFLOG2(3, " Backtracking to level %d, at trail index %d", jmplevel, dlevels[pivot]);
	savePhases();
	const uint32 from = dlevels[pivot];
	uint32 i = from, j = from;
	if (stable) {
		while (i < trail.size()) {
			const uint32 lit = trail[i++], v = ABS(lit);
			if (sp->level[v] > jmplevel) {
				cancelAssign(lit);
				if (!vsids.has(v)) vsids.insert(v);
			}
			else {
				assert(opts.chrono_en);
				trail[j] = lit;
				sp->index[v] = j++;
			}
		}
	}
	else {
		while (i < trail.size()) {
			const uint32 lit = trail[i++], v = ABS(lit);
			if (sp->level[v] > jmplevel) {
				cancelAssign(lit);
				if (vmtf.bumped() < bumps[v]) vmtf.update(v, bumps[v]);
			}
			else {
				assert(opts.chrono_en);
				trail[j] = lit;
				sp->index[v] = j++;
			}
		}
	}
	PFLOG2(3, "  %d literals kept (%d are saved) and %d are cancelled", j, j - from, trail.size() - j);
	trail.resize(j);
	if (sp->propagated > from) sp->propagated = from;
	dlevels.resize(pivot);
	assert(DL() == jmplevel);
}