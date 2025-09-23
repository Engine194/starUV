import React, { useEffect } from "react";
import { useI18nContext } from "../../contexts/i18nContext";
import classes from "./cycles.module.css";
import ListCycle from "./_listCycle";
import { useDispatch, useStoreContext } from "../../contexts/storeContext";
import { STORE_ACTION_TYPES } from "../../contexts/actions";

const Cycles = () => {
  const t = useI18nContext();
  const { cycles } = useStoreContext();
  const dispatch = useDispatch();

  useEffect(() => {
    fetch(`${window.location.origin}/config`)
      .then((res) => res.json())
      .then((data) => {
        dispatch({
          type: STORE_ACTION_TYPES.GET_CYCLES,
          payload: data?.cycles || [],
        });
      });
  }, [dispatch]);

  return (
    <section className={classes.root}>
      <h2 className={classes.heading}>{t({ id: "cycles", mask: "Cycles" })}</h2>
      <ListCycle data={cycles} />
    </section>
  );
};

export default Cycles;
