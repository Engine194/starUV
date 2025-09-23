import { STORE_ACTION_TYPES } from "../actions/storeContext.action";

export const initState = {
  language: "vi",
  loading: false,
  cycles: [],
};

export const storeReducer = (state, action) => {
  const { type, payload } = action;
  switch (type) {
    case STORE_ACTION_TYPES.GET_LANGUAGE:
    case STORE_ACTION_TYPES.UPDATE_LANGUAGE:
      return { ...state, language: payload };
    case STORE_ACTION_TYPES.START_LOADING:
      return { ...state, loading: true };
    case STORE_ACTION_TYPES.END_LOADING:
      return { ...state, loading: false };
    case STORE_ACTION_TYPES.GET_CYCLES:
      return { ...state, cycles: [...(payload || [])] };
    case STORE_ACTION_TYPES.ADD_CYCLE:
      return { ...state, cycles: [...state.cycles, payload] };
    case STORE_ACTION_TYPES.UPDATE_CYCLE: {
      const cycleId = payload.id;
      const findCycle = state.cycles.findIndex(({ id }) => id === cycleId);
      if (findCycle >= 0) {
        const nextCycles = [...state.cycles];
        nextCycles.splice(findCycle, 1, { ...payload });
        return { ...state, cycles: nextCycles };
      } else {
        return state;
      }
    }
    default:
      return state;
  }
};
