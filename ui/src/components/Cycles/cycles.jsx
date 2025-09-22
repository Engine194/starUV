import React from "react";
import { useI18nContext } from "../../contexts/i18nContext";
import classes from "./cycles.module.css";
import ListCycle from "./_listCycle";
import { useStoreContext } from "../../contexts/storeContext";

const Cycles = () => {
  const t = useI18nContext();
  const { cycles } = useStoreContext();
  return (
    <section className={classes.root}>
      <h2 className={classes.heading}>{t({ id: "cycles", mask: "Cycles" })}</h2>
      <ListCycle data={cycles} />
      {/* <form className={classes.timeForm} onSubmit={(e) => e.preventDefault()}>
        <p className={classes.explain}>
          {t({ id: "time.explain", mask: "Hour : Minute : Second" })}
        </p>
        <span></span>
        <div className={classes.timeDisplay}>
          <input name="hour" type="number" min={0} max={23} />
          <span>{":"}</span>
          <input name="minute" type="number" min={0} max={59} />
          <span>{":"}</span>
          <input name="second" type="number" min={0} max={59} />
        </div>
        <button type="submit">{t({ id: "save" })}</button>
      </form> */}
    </section>
  );
};

export default Cycles;
