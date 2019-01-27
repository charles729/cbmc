/*******************************************************************\

Module: Fault Localization for Goto Symex

Author: Peter Schrammel

\*******************************************************************/

/// \file
/// Fault Localization for Goto Symex

#include "goto_symex_fault_localizer.h"

#include <solvers/prop/prop_conv.h>

goto_symex_fault_localizert::goto_symex_fault_localizert(
  const optionst &options,
  ui_message_handlert &ui_message_handler,
  const symex_target_equationt &equation,
  prop_convt &solver)
  : options(options),
    ui_message_handler(ui_message_handler),
    equation(equation),
    solver(solver)
{
}

fault_location_infot goto_symex_fault_localizert::
operator()(const irep_idt &failed_property_id)
{
  fault_location_infot fault_location;
  localization_pointst localization_points;
  const auto &failed_step =
    collect_guards(failed_property_id, localization_points, fault_location);

  if(!localization_points.empty())
  {
    messaget log(ui_message_handler);
    log.status() << "Localizing fault" << messaget::eom;

    // pick localization method
    //  if(options.get_option("localize-faults-method")=="TBD")
    localize_linear(failed_step, localization_points);
  }

  return fault_location;
}

const symex_target_equationt::SSA_stept &
goto_symex_fault_localizert::collect_guards(
  const irep_idt &failed_property_id,
  localization_pointst &localization_points,
  fault_location_infot &fault_location)
{
  for(const auto &step : equation.SSA_steps)
  {
    if(
      step.is_assignment() &&
      step.assignment_type == symex_targett::assignment_typet::STATE &&
      !step.ignore)
    {
      if(!step.guard_literal.is_constant())
      {
        auto emplace_result = fault_location.scores.emplace(step.source.pc, 0);
        localization_points.emplace(step.guard_literal, emplace_result.first);
      }
    }

    // reached failed assertion?
    if(step.is_assert() && step.get_property_id() == failed_property_id)
      return step;
  }
  UNREACHABLE;
}

bool goto_symex_fault_localizert::check(
  const symex_target_equationt::SSA_stept &failed_step,
  const localization_pointst &localization_points,
  const localization_points_valuet &value)
{
  PRECONDITION(value.size() == localization_points.size());
  bvt assumptions;
  localization_points_valuet::const_iterator v_it = value.begin();
  for(const auto &l : localization_points)
  {
    if(v_it->is_true())
      assumptions.push_back(l.first);
    else if(v_it->is_false())
      assumptions.push_back(!l.first);
    ++v_it;
  }

  // lock the failed assertion
  assumptions.push_back(!failed_step.cond_literal);

  solver.set_assumptions(assumptions);

  if(solver() == decision_proceduret::resultt::D_SATISFIABLE)
    return false;

  return true;
}

void goto_symex_fault_localizert::update_scores(
  const localization_pointst &localization_points)
{
  for(auto &l : localization_points)
  {
    if(solver.l_get(l.first).is_true())
    {
      l.second->second++;
    }
    else if(solver.l_get(l.first).is_false() && l.second->second > 0)
    {
      l.second->second--;
    }
  }
}

void goto_symex_fault_localizert::localize_linear(
  const symex_target_equationt::SSA_stept &failed_step,
  const localization_pointst &localization_points)
{
  localization_points_valuet v;
  v.resize(localization_points.size());

  for(std::size_t i = 0; i < localization_points.size(); ++i)
    v[i] = tvt(tvt::tv_enumt::TV_UNKNOWN);

  for(std::size_t i = 0; i < v.size(); ++i)
  {
    v[i] = tvt(tvt::tv_enumt::TV_TRUE);
    if(!check(failed_step, localization_points, v))
      update_scores(localization_points);

    v[i] = tvt(tvt::tv_enumt::TV_FALSE);
    if(!check(failed_step, localization_points, v))
      update_scores(localization_points);

    v[i] = tvt(tvt::tv_enumt::TV_UNKNOWN);
  }

  // clear assumptions
  bvt assumptions;
  solver.set_assumptions(assumptions);
}
