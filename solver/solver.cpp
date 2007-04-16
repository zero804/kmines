/*
 * Copyright (c) 2001 Mikhail Kourinny (mkourinny@yahoo.com)
 * Copyright (c) 2002 Nicolas HADACEK  (hadacek@kde.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "solver.h"
#include "solver.moc"

#include <algorithm>
#include <assert.h>

#include <QTimer>
#include <QLayout>
#include <QLabel>
//Added by qt3to4:
#include <QVBoxLayout>
#include <qprogressbar.h>

#include <klocale.h>
#include <KStandardGuiItem>

#include "headerP.h"


//-----------------------------------------------------------------------------
class SolverPrivate
{
 public:
    SolverPrivate() : facts(0), rules(0) {}
    ~SolverPrivate() {
        delete facts;
        delete rules;
    }

    AdviseFast::FactSet *facts;
    AdviseFast::RuleSet *rules;
#ifdef DEBUG
    unsigned long t0, t;
#endif
};

Solver::Solver(QObject *parent)
    : QObject(parent)
{
    d = new SolverPrivate;

#ifdef DEBUG
#define PRINT_ELAPSED(purpose) \
    d->t = time(0); \
    cout << "Spent " << d->t - d->t0 << " seconds on " purpose << endl; \
    d->t0 = d->t;

#endif
}

Solver::~Solver()
{
    delete d;
}

Coord Solver::advise(BaseField &field, float &probability)
{
    Coord point;
    probability = 1;
    delete d->facts;
    d->facts = new AdviseFast::FactSet(&field);
    delete d->rules;
    d->rules = new AdviseFast::RuleSet(d->facts);

	if( AdviseFast::adviseFast(&point, d->facts, d->rules) ) return point;

	CoordSet surePoints;
	AdviseFull::ProbabilityMap probabilities;
	AdviseFull::adviseFull(d->facts, &surePoints, &probabilities);

    // return one of the sure point (random choice to limit the tropism) [NH]
	if( !surePoints.empty() ) {
        KRandomSequence r;
        uint k = r.getLong(surePoints.size());
        CoordSet::iterator it = surePoints.begin();
        for (uint i=0; i<k; i++) ++it;
        return *it;
    }

	// Just a minimum probability logic here
	if( !probabilities.empty() ){
		probability = probabilities.begin()->first;
		return probabilities.begin()->second;
	}

	// Otherwise the Field is already solved :)
	return Coord(-1,-1);
}

void Solver::solve(BaseField &field, bool noGuess)
{
    _field = &field;
    initSolve(false, noGuess);
}

bool Solver::initSolve(bool oneStep, bool noGuess)
{
    _inOneStep = oneStep;
    _noGuess = noGuess;
    delete d->facts;
    d->facts = new AdviseFast::FactSet(_field);
    delete d->rules;
    d->rules = new AdviseFast::RuleSet(d->facts);
#ifdef DEBUG
    d->t0 = time(0);
#endif
    return solveStep();
}

bool Solver::solveStep()
{
    if ( _field->isSolved() ) {
        emit solvingDone(true);
        return true;
    }

    d->rules->solve();

#ifdef DEBUG
	PRINT_ELAPSED("fast rules")
#endif

    if( _field->isSolved() ) {
        emit solvingDone(true);
        return true;
    }

    CoordSet surePoints;
	AdviseFull::ProbabilityMap probabilities;
	AdviseFull::adviseFull(d->facts, &surePoints, &probabilities);

#ifdef DEBUG
	PRINT_ELAPSED("full rules")
#endif

	if(!surePoints.empty()){
		CoordSet::iterator i;
		for(i=surePoints.begin(); i!=surePoints.end(); ++i) {
            bool b = d->rules->reveal(*i);
			assert(b);
        }
	} else if ( !_noGuess ) {
#ifdef DEBUG
	cout << "Applying heuristics!" << endl;
	cout << *_field << endl;
#endif
        // Minimum probability logic
        assert(!probabilities.empty());
#ifdef DEBUG
		AdviseFull::ProbabilityMap::iterator i=probabilities.begin();
		cout << "Probability is " << i->first << endl;
#endif
		bool success = d->rules->reveal(probabilities.begin()->second);
        if ( !success ) {
            emit solvingDone(false);
            return false;
        }
    }

    if (_inOneStep) return solveStep();
    else QTimer::singleShot(0, this, SLOT(solveStep()));
    return false;
}

bool Solver::solveOneStep(BaseField &field)
{
    _field = &field;
    return initSolve(true, false);
}
