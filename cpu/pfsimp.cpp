/***********************************************************************
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
************************************************************************/

#include "pfsimp.h"
#include "pfsimpopts.h"

void ParaFROST::opt_simp()
{
	assert(pre_en);
	simp_perf_en = opt_simp_perf_en;
	ve_en = opt_ve_en || opt_ve_plus_en;
	ve_plus_en = opt_ve_plus_en;
	sub_en = opt_sub_en;
	bce_en = opt_bce_en;
	hre_en = opt_hre_en;
	all_en = opt_all_en;
	phases = opt_phases;
	mu_pos = opt_mu_pos;
	mu_neg = opt_mu_neg;
	cnf_free_freq = opt_cnf_free;
	if (!phases && ve_en) phases = 1; // at least 1 phase needed for BVE(+)
	if (phases && !ve_en) phases = 0;
}

void ParaFROST::hist(const SCNF& cnf, bool rst)
{
	if (cnf.size() == 0) return;
	if (rst) { for (int i = 0; i < occurs.size(); i++) occurs[i].reset(); }
	for (int i = 0; i < cnf.size(); i++) {
		if (cnf[i]->status() != DELETED)
			clHist(cnf[i]);
	}
}

void ParaFROST::var_reorder()
{
	timer->start();
	scores.resize(nOrgVars());
	occurs.resize(nOrgVars());
	assert(!scnf.empty());
	hist(scnf, true);
	for (uint32 v = 0; v < nOrgVars(); v++) {
		scores[v].v = v;
		if (!occurs[v].ps || !occurs[v].ns) 
			scores[v].sc = occurs[v].ps | occurs[v].ns;
		else
			scores[v].sc = occurs[v].ps * occurs[v].ns;
	}
	Sort(scores, VAR_CMP());
	if (verbose == 4) {
		printf("c | Ordered #occurs: \n");
		for (uint32 i = 0; i < nOrgVars(); i++) {
			printf("c | var(%d).occurs = %d\n", scores[i].v + 1, scores[i].sc);
		}
	}
	timer->stop();
	timer->vo += timer->CPU_time();
}

void ParaFROST::attachClause(S_REF c, const bool& added)
{
	assert(*c != NULL);
	assert(c->size());
	assert(c->status() == UNKNOWN);
	if (added) {
		if (c->size() == 1 && sol->assign(V2IDX(**c)) == UNDEFINED) enqueue(**c);
		else {
			c->set_status(LEARNT);
			c->calcSig();
			scnf.push(c);
			cnf_stats.global_n_cls++;
			cnf_stats.global_n_lits += c->size();
		}
		for (LIT_POS l = 0; l < c->size(); l++) ot[(*c)[l]].push(c); // attach to OT
		if (proof_en) {
			write_proof('a');
			write_proof(*c, c->size());
			write_proof(0);
		}
	}
	else {
		assert(c->size() > 1);
		c->set_status(ORIGINAL);
		c->calcSig();
		Sort(c->d_ptr(), c->size());
		scnf[cnf_stats.global_n_cls++] = c;
		cnf_stats.global_n_lits += c->size();
	}
}

void ParaFROST::reattachClause(B_REF c)
{
	CL_LEN sz = c->size();
	assert(sz > 1);
	assert(*c != NULL);
	assert(**c > 0 && *(*c + 1) > 0);
	assert(**c <= UINT32_MAX && *(*c + 1) <= UINT32_MAX);
	wt[FLIP(**c)].push(WATCH(c, *(*c + 1)));
	wt[FLIP(*(*c + 1))].push(WATCH(c, **c));
	if (sz == 2) {
		cnf_stats.global_n_bins++;
		bins.push(c); // for pdm_init histogram
	}
	else {
		c->set_status(ORIGINAL);
		orgs.push(c);
		cnf_stats.global_n_cls++;
		cnf_stats.global_n_lits += sz;
		if (sz > cnf_stats.max_org_cl_width) cnf_stats.max_org_cl_width = sz;
	}
}

void ParaFROST::shrinkCNF(bool countVars)
{
	cnf_stats.global_n_cls = 0;
	cnf_stats.global_n_lits = 0;
	if (countVars) {
		cnf_stats.global_n_del_vars = 0;
		for (int i = 0; i < scnf.size(); i++) {
			S_REF c = scnf[i];
			if (c->status() == DELETED) delete c;
			else {
				assert(c->size() > 1);
				for (LIT_POS k = 0; k < c->size(); k++) sp->seen[V2IDX(c->lit(k))] = true;
				scnf[cnf_stats.global_n_cls++] = c;
				cnf_stats.global_n_lits += c->size();
			}
		}
		for (uint32 v = 0; v < nOrgVars(); v++) {
			if (!sp->seen[v]) {
				cnf_stats.global_n_del_vars++;
				if (!sp->lock[v] && !pv->melted[v]) removed.push(V2D(v + 1)); // disappeared
			}
			else sp->seen[v] = 0;
		}
	}
	else {
		for (int i = 0; i < scnf.size(); i++) {
			S_REF c = scnf[i];
			if (c->status() == DELETED) delete c;
			else {
				assert(c->size() > 1);
				scnf[cnf_stats.global_n_cls++] = c;
				cnf_stats.global_n_lits += c->size();
			}
		}
	}
	scnf.resize(cnf_stats.global_n_cls);
}

void ParaFROST::strengthen(S_REF c, const uint32& self_lit)
{
	uint32 sig = 0;
	CL_LEN n = 0;
	bool check = false;
	for (LIT_POS k = 0; k < c->size(); k++) {
		uint32 lit = c->lit(k);
		if (lit != self_lit) {
			(*c)[n++] = lit;
			sig |= mapHash(lit);
		}
		else check = true;
	}
	assert(check);
	assert(n == c->size() - 1);
	assert(c->hasZero() < 0);
	assert(c->isSorted());
	c->set_sig(sig);
	c->pop();
	if (proof_en) {
		write_proof('a');
		write_proof(*c, c->size());
		write_proof(0);
	}
	if (c->size() == 1 && sol->assign(V2IDX(**c)) == UNDEFINED) enqueue(**c);
}

bool ParaFROST::propClause(S_REF c, const uint32& f_assign)
{
	uint32 sig = 0;
	CL_LEN n = 0;
	bool check = false;
	for (LIT_POS k = 0; k < c->size(); k++) {
		uint32 lit = c->lit(k);
		if (lit != f_assign) {
			if (sol->assign(V2IDX(lit)) == !ISNEG(lit)) 
				return true;
			(*c)[n++] = lit;
			sig |= mapHash(lit);
		}
		else check = true;
	}
	assert(check);
	assert(n == c->size() - 1);
	assert(c->hasZero() < 0);
	assert(c->isSorted());
	c->set_sig(sig);
	c->pop();
	return false;
}

CNF_STATE ParaFROST::prop()
{
	while (sp->trail_head < sp->trail_size) { // propagate units
		uint32 assign = sp->trail[sp->trail_head++], assign_idx = V2IDX(assign), f_assign = FLIP(assign);
		assert(assign > 0);
		removed.push(assign);
		if (verbose >= 4) printf("c | Propagating assign("), printLit(assign), printf("):\n"), printOL(ot[assign]), printOL(ot[f_assign]);
		// remove satisfied
		for (int i = 0; i < ot[assign].size(); i++) ot[assign][i]->set_status(DELETED);
		// reduce unsatisfied
		for (int i = 0; i < ot[f_assign].size(); i++) {
			S_REF c = ot[f_assign][i];
			//c->print();
			assert(c->size());
			if (c->status() == DELETED || propClause(c, f_assign)) continue; // clause satisfied
			// clause is unit or conflict
			// Note: attach & strengthen don't check for conflict before enqueue
			if (c->size() == 0) return UNSAT;  
			if (c->size() == 1) { 
				assert(c->w0_lit());
				uint32 unit = c->w0_lit(), unit_idx = V2IDX(unit);
				if (sol->assign(unit_idx) == UNDEFINED) enqueue(unit);
				else return UNSAT;  // conflict on top level
			}
		}
		if (verbose >= 4) printOL(ot[assign]), printOL(ot[f_assign]);
		// delete assign lists
		ot[assign].clear(true), ot[f_assign].clear(true);
	}
	return UNSOLVED;
}

void ParaFROST::create_ot(bool rst)
{
	timer->start();
	// reset ot
	if (rst) {
		for (uint32 v = 0; v < nOrgVars(); v++) { 
			uint32 p = V2D(v + 1); 
			ot[p].clear(); 
			ot[NEG(p)].clear();
		}
	}
	// create ot
	for (int i = 0; i < scnf.size(); i++) {
		SCLAUSE& c = *scnf[i];
		if (c.status() != DELETED) {
			assert(c.status() != UNKNOWN);
			assert(c.size() > 1);
			for (LIT_POS l = 0; l < c.size(); l++) { 
				assert(c[l] != 0);
				ot[c[l]].push(scnf[i]);
			}
		}
	}
	assert(consistent(scnf, ot));
	timer->stop();
	timer->ot += timer->CPU_time();
}

void ParaFROST::_simplify()
{
	C_REF ref = BCP();
	assert(ref == NULL); // at this point BCP cannot return a conflict!
	if (sp->trail_size == sp->trail_offset) return;
	assert(sp->trail_size > 0 && DL() == ROOT_LEVEL);
	assert(sp->trail_head == sp->trail_size);
	assert(sol->assign(V2IDX(sp->trail[sp->trail_size - 1])) != UNDEFINED);
	if (verbose >= 2) printf("c | Simplifying CNF..");
	timer->start();
	// reduce watch table
	for (int i = sp->trail_offset; i < sp->trail_size; i++) {
		uint32 assign = sp->trail[i], assign_f = FLIP(assign);
		// remove bins from WT
		WL& ws = wt[assign_f];
		for (int j = 0; j < ws.size(); j++) {
			B_REF c = (B_REF)ws[j].c_ref;
			if (c->orgBin()) { // original binary
				assert(c->size() == 2);
				assert(c->status() == UNKNOWN);
				//printf("c | Removing bin clause "); c->print();
				uint32 w_lit = (assign == c->w1_lit()) ? c->w0_lit() : c->w1_lit();
				remWatch(wt[FLIP(w_lit)], c);
				delete c;
				cnf_stats.global_n_bins--;
			}
		}
		// remove root-level watch lists
		wt[assign].clear(true);
		wt[assign_f].clear(true);
		uint32 assign_idx = V2IDX(assign);
		removed.push(assign); // save in case preprocessing mapped variables
	}
	// simplify input CNF
	cnf_stats.global_n_cls = simplify(orgs);
	orgs.resize(cnf_stats.global_n_cls);
	cnf_stats.global_n_del_vars += (sp->trail_size - sp->trail_offset);
	sp->trail_offset = sp->trail_size;
	timer->stop();
	timer->red += timer->CPU_time();
	if (verbose >= 2) printf(" ==> done\n");
	//print(wt);
}

void ParaFROST::extractBins()
{
	if (pre_delay) {
		if (nOrgBins()) {
			bins.clear(), bins.resize(nBins());  // for garbage collection
			uint32 nbins = 0;
			for (uint32 v = 0; v < nOrgVars(); v++) {
				uint32 p = V2D(v + 1), n = NEG(p);
				WL& posBins = wt[n];
				for (int j = 0; j < posBins.size(); j++) {
					assert(posBins[j].c_ref != NULL);
					B_REF c = (B_REF)posBins[j].c_ref;
					if (c->size() == 2 && !c->orgBin() && !c->molten()) { // collect learnt bins
						bins[nbins++] = c;
						c->melt();
					}
					if (c->orgBin()) {
						assert(c->status() == UNKNOWN);
						S_REF sc = new SCLAUSE();
						sc->copyLitsFrom(*c, 2);
						attachClause(sc, false);
						c->reset_orgBin();
						c->melt();
						bins[nbins++] = c;
					}
				}
				WL& negBins = wt[p];
				for (int j = 0; j < negBins.size(); j++) {
					assert(negBins[j].c_ref != NULL);
					B_REF c = (B_REF)negBins[j].c_ref;
					if (c->size() == 2 && !c->orgBin() && !c->molten()) {
						bins[nbins++] = c;
						c->melt();
					}
					if (c->orgBin()) {
						assert(c->status() == UNKNOWN);
						S_REF sc = new SCLAUSE();
						sc->copyLitsFrom(*c, 2);
						attachClause(sc, false);
						c->reset_orgBin();
						c->melt();
						bins[nbins++] = c;
					}
				}
			}
			for (uint32 i = 0; i < nbins; i++) delete bins[i];
		}
	}
	else {
		for (int i = 0; i < bins.size(); i++) {
			if (bins[i]->orgBin()) {
				assert(bins[i]->status() == UNKNOWN);
				S_REF sc = new SCLAUSE();
				sc->copyLitsFrom(*bins[i], 2);
				attachClause(sc, false);
			}
			delete bins[i];
		}
	}
	bins.clear(true);
	wt.clear(true);
}

bool ParaFROST::awaken()
{ 
	if (pre_delay) _simplify(); // bcp/simplify any remained facts at root level
	// alloc memory for occur. table
	size_t maxVars = V2D(size_t(nOrgVars()) + 1);
	double ot_cap = (double)maxVars * sizeof(S_REF);
	sysMemCons += ot_cap;
	if (sysMemCons >= sysMemTot) { // to catch memout problems before exception does
		if (verbose > 1) cout << "\nc | Not enough system memory for occur. table (Max: " << sysMemTot / MBYTE << ", Consumed: " << sysMemCons / MBYTE << " MB)" << endl;
		sysMemCons -= ot_cap;
		return false;
	}
	ot.resize(maxVars);
	// alloc memory for scnf
	assert(scnf.empty());
	assert(orgs.size() == nClauses());
	int scnf_sz = nOrgClauses();
	double scnf_cap = (double)scnf_sz * sizeof(S_REF) + nOrgLits() * sizeof(uint32);
	sysMemCons += scnf_cap;
	if (sysMemCons >= sysMemTot) {
		if (verbose > 1) cout << "\nc | Not enough system memory for SCNF (Max: " << sysMemTot / MBYTE << ", Consumed: " << scnf_cap / MBYTE << " MB)" << endl;
		sysMemCons -= scnf_cap;
		return false;
	}
	scnf.resize(scnf_sz);
	cnf_stats.global_n_cls = 0;
	cnf_stats.global_n_lits = 0;
	extractBins(); 
	// append k-size orgs
	for (int i = 0; i < orgs.size(); i++) {
		assert(orgs[i]->size() > 2);
		S_REF sc = new SCLAUSE();
		sc->copyLitsFrom(*orgs[i], orgs[i]->size());
		attachClause(sc, false);
		assert(orgs[i]->status() == ORIGINAL);
		delete orgs[i];
	}
	orgs.clear(true);
	for (int i = 0; i < learnts.size(); i++) delete learnts[i];
	learnts.clear();
	scnf.resize(nClauses());
	for (uint32 v = 0; v < nOrgVars(); v++) { sp->lock[v] = false; sp->seen[v] = false; sol->init(v); }
	sp->reset_trail();
	print_pstats();
	return true;
}

void ParaFROST::preprocess()
{
	/********************/
	/* Warming up sigma */
	/********************/
	if (!awaken()) { pre_en = lpre_en = false; return; } // still lazy!
	uint32 simpVars = nRemVars();
	/********************************/
	/*      1st-stage reduction     */
	/********************************/
	pv->mu_inc = 0;
	int64 lits_before = nLiterals();
	int64 lits_diff = INT64_MAX;
	int phase = 0;
	while (lits_diff >= LIT_REM_THR && phase < phases) {
		if (verbose > 1) cout << "c |\t\tPhase-" << phase << " Variable Elections" << endl;
		LCVE(); 
		if (pv->nPVs <= MIN_PARALLEL_VARS) {
			if (verbose > 1) {
				cout << "c | Number of parallel variables is very small --> exit procedure." << endl;
				cout << "c |-----------------------------------------------------------------|" << endl;
			}
			break;
		}
		VE(); // perform an extended BVE
		cnt_lits(); // count number of remaining literals & clauses
		if (phase >= 2 && cnf_stats.n_lits_after >= lits_before) {
			if (verbose > 1) {
				cout << "c | WARNING: literals are increasing --> exit procedure." << endl;
				cout << "c |-----------------------------------------------------------------|" << endl;
			}
			break;
		}
		lits_diff = (lits_before > cnf_stats.n_lits_after) ? lits_before - cnf_stats.n_lits_after : cnf_stats.n_lits_after - lits_before;
		if (lits_diff && phase % cnf_free_freq == 0) shrinkCNF();
		phase++;
		pv->mu_inc++; // to increase the temperature
		lits_before = cnf_stats.n_lits_after;
		if (verbose > 1) cout << "c |-----------------------------------------------------------------|" << endl;
	} // end-of-while
	/********************************/
	/*      2nd-stage reduction     */
	/********************************/
	if (sub_en | hre_en | bce_en | all_en) {
		if (verbose > 1) cout << "c | 2nd-Stage Clause Eliminations.." << endl;
		int t_p = mu_pos, t_n = mu_neg;
		while (t_p < CE_POS_LMT && t_n < CE_NEG_LMT) pv->mu_inc++, t_p <<= pv->mu_inc, t_n <<= pv->mu_inc;
		LCVE();
		if (all_en | sub_en) SUB();
		if (all_en | bce_en) BCE();
		if (all_en | hre_en) HRE();
	}
	/********************************/
	/*           Write Back         */
	/********************************/
	ot.clear(true);
	shrinkCNF(true); 
	print_pstats();
	cnf_stats.n_org_cls = nClauses();
	cnf_stats.n_org_lits = nLiterals();
	if (nRemVars() - simpVars != 0) { // variables are removed by preprocess? then map
		mapped = true;
		mappedVars.resize(nOrgVars() + 1, 0);
		reverseVars.resize(nOrgVars() + 1, 0);
		cnf_stats.n_org_vars -= nRemVars(); // update nr. variables
		cleanSlate();
	}
	else { // no mapping is needed
		assert(nRemVars() == simpVars);
		cleanSlate();
	}
}

void ParaFROST::cleanSlate()
{
	cnf_stats.global_n_bins = 0;
	cnf_stats.global_n_cls = 0;
	cnf_stats.global_n_lits = 0;
	cnf_stats.max_org_cl_width = 0;
	WT_alloc(true);
	if (mapped) {
		cnf_stats.global_n_del_vars = 0;
		for (int i = 0; i < scnf.size(); i++) {
			assert(scnf[i]->status() == ORIGINAL || scnf[i]->status() == LEARNT);
			assert(scnf[i]->size() > 1);
			B_REF org = new BCLAUSE(scnf[i]->size());
			mapClause(*scnf[i], *org);
			reattachClause(org);
			delete scnf[i];
		}
		scnf.clear(true);
		mappedVars.clear(true);
		solver_alloc(true);
		init();
	}
	else {
		for (int i = 0; i < scnf.size(); i++) {
			assert(scnf[i]->status() == ORIGINAL);
			assert(scnf[i]->size() > 1);
			B_REF org = new BCLAUSE();
			org->copyLitsFrom(*scnf[i], scnf[i]->size());
			reattachClause(org);
			delete scnf[i];
		}
		scnf.clear(true);
		init(false);
		for (int i = 0; i < removed.size(); i++) {
			uint32 remVar = V2IDX(removed[i]);
			if (var_heap->has(remVar)) var_heap->remove(remVar);
			sp->lock[remVar] = true;
			source[remVar] = NULL;
			sol->set_assign(remVar, !ISNEG(removed[i]));
			sol->set_level(remVar, ROOT_LEVEL);
		}
	}
	assert(consistent(orgs, wt));
}

void ParaFROST::depFreeze(const OL& ol, const uint32& cand, const int& p_temp, const int& n_temp)
{
	for (int i = 0; i < ol.size(); i++) {
		SCLAUSE& c = *ol[i];
		for (LIT_POS k = 0; k < c.size(); k++) {
			register uint32 v = V2IDX(c[k]);
			if (v != cand && (occurs[v].ps < p_temp || occurs[v].ns < n_temp)) sp->frozen[v] = true;
		}
	}
}

void ParaFROST::LCVE()
{
	// recreation
	var_reorder();
	create_ot();
	// extended LCVE
	timer->start();
	pv->nPVs = 0;
	for (int v = 0; v < nOrgVars(); v++) {
		uint32 cand = scores[v].v;
		uint32 p = V2D(cand + 1), n = NEG(p);
		if (sp->frozen[cand]) continue;
		register int poss_sz = ot[p].size(), negs_sz = ot[n].size();
		assert(poss_sz == occurs[cand].ps);
		assert(negs_sz == occurs[cand].ns);
		if (poss_sz == 0 || negs_sz == 0) continue; 
		int pos_temp = mu_pos << pv->mu_inc, neg_temp = mu_neg << pv->mu_inc;
		if (poss_sz >= pos_temp && negs_sz >= neg_temp) break;
		assert(!pv->melted[cand]);
		pv->PVs[pv->nPVs++] = cand;
		depFreeze(ot[p], cand, pos_temp, neg_temp);
		depFreeze(ot[n], cand, pos_temp, neg_temp);
	}
	for (int v = 0; v < pv->nPVs; v++) assert(!sp->frozen[pv->PVs[v]]);
	if (verbose >= 3) {
		printf("c | PLCVs (n = %d): \n", pv->nPVs);
		printVars(pv->PVs, pv->nPVs);
	}
	bool* f = sp->frozen, * f_end = f + nOrgVars();
	while (f != f_end) *f++ = 0;
	timer->stop();
	timer->lcve += timer->CPU_time();
}

void ParaFROST::bve()
{
	timer->start();
	for (int v = 0; v < pv->nPVs; v++) {
		register uint32 x = pv->PVs[v];
		assert(!pv->melted[x]);
		uint32 p = V2D(x + 1), n = NEG(p);
		if (ot[p].size() == 1 || ot[n].size() == 1) { 
			resolve_x(x + 1, ot[p], ot[n]); 
			pv->melted[x] = true;
			removed.push(V2D(x + 1));
			ot[p].clear(true), ot[n].clear(true);
		}
		else if (gateReasoning_x(p, ot[p], ot[n]) || mayResolve_x(x + 1, ot[p], ot[n])) {
			pv->melted[x] = true;
			removed.push(V2D(x + 1));
			ot[p].clear(true), ot[n].clear(true);
		}
	}
	if (prop() == UNSAT) {
		wrapUp(UNSAT);
		this->~ParaFROST();
		exit(EXIT_SUCCESS);
	}
	timer->stop();
	timer->bve += timer->CPU_time();
}

void ParaFROST::VE()
{
	assert(pv->nPVs > 0);
	if (verbose > 1) cout << "c | Eliminating variables for round-0." << endl;
	bve();
	if (verbose > 1) { eval_reds(); printf("c |\t\tBVE Reductions\n"); logReductions(); }
	if (ve_plus_en) {
		cnt_lits();
		int64 lits_before = cnf_stats.n_lits_after;
		int64 lits_removed = nLiterals() - cnf_stats.n_lits_after;
		int64 lits_removed_pre = lits_removed;
		while (lits_removed > LIT_REM_THR) {
			// filter elected variables
			int n = 0;
			for (int v = 0; v < pv->nPVs; v++) {
				uint32 x = pv->PVs[v];
				if (!pv->melted[x]) pv->PVs[n++] = x;
			}
			pv->nPVs = n;
			if (pv->nPVs == 0) {
				if (verbose > 1) cout << "c | Nothing left to eliminate --> terminate procedure." << endl;
				break;
			}
			// HSE 
			if (verbose > 1) cout << "c | HSE-ing non-eliminated variables..";
			HSE();
			cnt_lits(); // count remained literals
			lits_removed = lits_before - cnf_stats.n_lits_after;
			lits_before = cnf_stats.n_lits_after;
			if (verbose > 1) cout << "(Literals removed: -" << lits_removed << ") ==> done." << endl;
			if (lits_removed <= LIT_REM_THR) break;
			// execute BVE again
			if (verbose > 1) cout << "c | Eliminating variables in new round..";
			create_ot(); // must be recreated to discard deleted clauses
			bve();
			cnt_lits(); // count remained literals
			lits_removed = (lits_before > cnf_stats.n_lits_after) ? lits_before - cnf_stats.n_lits_after : cnf_stats.n_lits_after - lits_before;
			if (verbose > 1) {
				if (lits_before > cnf_stats.n_lits_after) cout << "(literals reduction: -" << lits_removed << ") ==> done." << endl;
				else cout << "(literals reduction: +" << lits_removed << ") ==> done." << endl;
			}
			lits_before = cnf_stats.n_lits_after;
		}
		if (lits_removed_pre != lits_removed && verbose > 1) { eval_reds(); printf("c |\t\tBVE+ Reductions\n"); logReductions(); }
	}
}

void ParaFROST::HSE()
{
	timer->start();
	for (int v = 0; v < pv->nPVs; v++) {
		register uint32 x = pv->PVs[v];
		uint32 p = V2D(x + 1), n = NEG(p);
		if (!pv->melted[x]) self_sub_x(p, ot[p], ot[n]);
	}
	if (prop() == UNSAT) {
		wrapUp(UNSAT);
		this->~ParaFROST();
		exit(EXIT_SUCCESS);
	}
	timer->stop();
	timer->hse += timer->CPU_time();
}

void ParaFROST::SUB()
{
	if (verbose > 1) cout << "c | SUB-ing variables..";
	timer->start();
	for (int v = 0; v < pv->nPVs; v++) {
		register uint32 x = pv->PVs[v];
		assert(!pv->melted[x]);
		uint32 p = V2D(x + 1), n = NEG(p);
		self_sub_x(p, ot[p], ot[n]);
	}
	if (prop() == UNSAT) {
		wrapUp(UNSAT);
		this->~ParaFROST();
		exit(EXIT_SUCCESS);
	}
	timer->stop();
	timer->hse += timer->CPU_time();
	if (verbose > 1) cout << " ==> done." << endl;
	if (verbose > 1) { cnt_cls(); printf("c |\t\t\t SUB Reductions\n"); logReductions(); }
}

void ParaFROST::BCE()
{
	if (verbose > 1) cout << "c | Eliminating blocked clauses..";
	timer->start();
	for (int v = 0; v < pv->nPVs; v++) {
		register uint32 x = pv->PVs[v];
		assert(!pv->melted[x]);
		uint32 p = V2D(x + 1), n = NEG(p);
		blocked_x(x + 1, ot[p], ot[n]);
	}
	timer->stop();
	timer->bce += timer->CPU_time();
	if (verbose > 1) cout << " ==> done." << endl;
	if (verbose > 1) { cnt_cls(); printf("c |\t\t\t BCE Reductions\n"); logReductions(); }
}

void ParaFROST::HRE()
{
	if (verbose > 1) cout << "c | Eliminating hidden redundances..";
	timer->start();
	for (int v = 0; v < pv->nPVs; v++) {
		assert(!pv->melted[pv->PVs[v]]);
		uint32 posY = V2D(pv->PVs[v] + 1);
		OL& y_poss = ot[posY], & y_negs = ot[NEG(posY)];
		// do merging and apply forward equality check (on-the-fly) over resolvents
		for (int i = 0; i < y_poss.size(); i++) {
			if (y_poss[i]->status() == DELETED) continue;
			//printf("Pos(%d):", i); y_poss[i]->print();
			for (int j = 0; j < y_negs.size(); j++) {
				if (y_negs[j]->status() == DELETED) continue;
				//printf("Neg(%d):", j); y_negs[j]->print();
				uVector1D m_c;
				if (merge_hre(pv->PVs[v] + 1, y_poss[i], y_negs[j], m_c)) {
					uint32 m_sig = 0;
					for (int k = 0; k < m_c.size(); k++) m_sig |= mapHash(m_c[k]);
					for (int k = 0; k < m_c.size(); k++) {
						if (forward_equ(m_c, m_sig, ot[m_c[k]])) break;
					} // end-for (m_c)
				} // end-if (merge)
			} // end-for (negs_list)
		} // end-for (poss_list)
	}
	timer->stop();
	timer->hre += timer->CPU_time();
	if (verbose > 1) cout << " ==> done." << endl;
	if (verbose > 1) { cnt_cls(); printf("c |\t\t\t HRE Reductions\n"); logReductions(); }
}

bool ParaFROST::consistent(const SCNF& cnf, const OT& ot)
{
	for (int i = 0; i < cnf.size(); i++) {
		S_REF c = cnf[i];
		assert(c->size());
		//c->print();
		for (LIT_POS k = 0; k < c->size(); k++) {
			assert(c->lit(k));
			const OL& ol = ot[c->lit(k)];
			if (c->status() != DELETED) assert(ol.size());
			bool found = false;
			for (int j = 0; j < ol.size(); j++) {
				S_REF ref = ol[j];
				assert(ref->size());
				if (ref == c) {
					found = true;
					break;
				}
			}
			if (c->status() == DELETED && found) {
				printf(" c");
				c->print();
				printf(" found DELETED in list["); printLit(c->lit(k)); printf("]:\n");
				printOL(ol);
				return false;
			}
			if (c->status() != DELETED && !found) {
				printf(" c");
				c->print();
				printf(" NOT found in list["); printLit(c->lit(k)); printf("]:\n");
				printOL(ol);
				return false;
			}
		}
	}
	return true;
}