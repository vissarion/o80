

namespace internal
{
// this function calls the (correct overload) of the
// interpolate::intermediate_state function for each of the dimension of the
// state as encapsulated by the (tuple instance) values_ of the instances
// StateXd. It uses template recursivity to loop over the values encapsulated by
// the tuples.
template < size_t INDEX = 0, 
	     typename INCR, // o80::TimePoint (for speed and duration) or long int (for iteration)
	     typename TUPLE,  
	     size_t SIZE =
	     std::tuple_size_v<
	       std::remove_reference_t<TUPLE>>,
	     typename COMMAND_TYPE> // Speed, Duration or Iteration
  void intermediates(INCR&& start,
		     INCR&& now,
		     const TUPLE& start_state,
		     const TUPLE& previous_desired_state,
		     const TUPLE& target_state,
		     const COMMAND_TYPE &command, 
		     TUPLE& interpolated_state,
		     bool use_duration=false)
{
    static o80::Duration_us duration;

    // if the command is of type speed, then the speed is used only for
    // the first state dimension (INDEX=0),
    // other states use the inferred command duration
    if constexpr (INDEX == 0 && std::is_same<COMMAND_TYPE, o80::Speed>::value)
    {
        double init = static_cast<double>(std::get<0>(start_state));
        double end = static_cast<double>(std::get<0>(target_state));
        double diff = fabs(end - init);
        duration.value = static_cast<long int>(diff / command.value + 0.5);
        use_duration = true;
    }

    if constexpr (INDEX < SIZE)
    {
        auto value =
            o80::intermediate_state(std::forward<INCR>(start),
                                    std::forward<INCR>(now),
                                    std::get<INDEX>(start_state),
                                    std::get<INDEX>(previous_desired_state),
                                    std::get<INDEX>(target_state),
                                    command);
        std::get<INDEX>(interpolated_state) = value;
        // recursive call over the tuples
        if constexpr (INDEX + 1 < SIZE)
        {
            if (use_duration)
            {
                // originally a speed command, but converted to duration
                // command for indexes > 0
                intermediates<INDEX + 1>(std::forward<INCR>(start),
                                         std::forward<INCR>(now),
                                         start_state,
                                         previous_desired_state,
                                         target_state,
                                         duration,
                                         interpolated_state,
                                         use_duration);
            }
            else
            {
                intermediates<INDEX + 1>(std::forward<INCR>(start),
                                         std::forward<INCR>(now),
                                         start_state,
                                         previous_desired_state,
                                         target_state,
                                         command,
                                         interpolated_state);
            }
        }
    }
}

template <size_t INDEX = 0,
          typename TUPLE,
          size_t SIZE = std::tuple_size_v<std::remove_reference_t<TUPLE>>>
bool all_finished(const o80::TimePoint &start,
                  const o80::TimePoint &now,
                  const TUPLE &start_state,
                  const TUPLE &previous_desired_state,
                  const TUPLE &target_state,
                  const o80::Speed &speed)
{
    static bool finished = true;

    if constexpr (INDEX == 0)
    {
        finished = true;
    }

    if constexpr (INDEX < SIZE)
    {
        finished = o80::finished(start,
                                 now,
                                 std::get<INDEX>(start_state),
                                 std::get<INDEX>(previous_desired_state),
                                 std::get<INDEX>(target_state),
                                 speed);

        if (!finished) return false;

        if constexpr (INDEX + 1 < SIZE)
            all_finished<INDEX + 1>(start,
                                    now,
                                    start_state,
                                    previous_desired_state,
                                    target_state,
                                    speed);
    }

    return true;
}

}  // namespace internal

template <class Sub, typename... Args>
StateXd<Sub, Args...>::StateXd(Args... args)
    : values_(std::forward<Args>(args)...)
{
}

template <class Sub, typename... Args>
StateXd<Sub, Args...>::StateXd()
{
}

template <class Sub, typename... Args>
template <int INDEX>
typename std::tuple_element<INDEX, std::tuple<Args...>>::type
StateXd<Sub, Args...>::get() const
{
    return std::get<INDEX>(values_);
}

template <class Sub, typename... Args>
template <int INDEX>
void StateXd<Sub, Args...>::set(
    typename std::tuple_element<INDEX, std::tuple<Args...>>::type value)
{
    std::get<INDEX>(values_) = value;
}

template <class Sub, typename... Args>
bool StateXd<Sub, Args...>::finished(const o80::TimePoint &start,
                                     const o80::TimePoint &now,
                                     const Sub &start_state,
                                     const Sub &current_state,
                                     const Sub &previous_desired_state,
                                     const Sub &target_state,
                                     const o80::Speed &speed) const
{
    return internal::all_finished(start,
                                  now,
                                  start_state.values_,
                                  previous_desired_state.values_,
                                  target_state.values_,
                                  speed);
}

template <class Sub, typename... Args>
Sub StateXd<Sub, Args...>::intermediate_state(const o80::TimePoint &start,
                                              const o80::TimePoint &now,
                                              const Sub &start_state,
                                              const Sub &current_state,
                                              const Sub &previous_desired_state,
                                              const Sub &target_state,
                                              const o80::Speed &speed) const
{
    Sub interpolated_state;
    internal::intermediates(start,
                            now,
                            start_state.values_,
                            previous_desired_state.values_,
                            target_state.values_,
                            speed,
                            interpolated_state.values_);
    return interpolated_state;
}

template <class Sub, typename... Args>
Sub StateXd<Sub, Args...>::intermediate_state(
    const o80::TimePoint &start,
    const o80::TimePoint &now,
    const Sub &start_state,
    const Sub &current_state,
    const Sub &previous_desired_state,
    const Sub &target_state,
    const o80::Duration_us &duration) const
{
    Sub interpolated_state;
    internal::intermediates(start,
                            now,
                            start_state.values_,
                            previous_desired_state.values_,
                            target_state.values_,
                            duration,
                            interpolated_state.values_);
    return interpolated_state;
}

template <class Sub, typename... Args>
Sub StateXd<Sub, Args...>::intermediate_state(
    long int start_iteration,
    long int current_iteration,
    const Sub &start_state,
    const Sub &current_state,
    const Sub &previous_desired_state,
    const Sub &target_state,
    const o80::Iteration &iteration) const
{
    Sub interpolated_state;
    internal::intermediates(start_iteration,
                            current_iteration,
                            start_state.values_,
                            previous_desired_state.values_,
                            target_state.values_,
                            iteration,
                            interpolated_state.values_);
    return interpolated_state;
}