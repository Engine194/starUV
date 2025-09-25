import React from "react";
import { useI18nContext } from "../../contexts/i18nContext";
import classes from "./setTime.module.css";

const SetTime = () => {
  const t = useI18nContext();
  return (
    <section className={classes.root}>
      <h2 className={classes.heading}>
        {t({ id: "time.setting", mask: "Time Setting" })}
      </h2>
      <form className={classes.timeForm} onSubmit={(e) => e.preventDefault()}>
        <p className={classes.explain}>
          {t({ id: "date.explain", mask: "Year / month / day" })}
        </p>
        <span/>
        <div className={classes.timeDisplay}>
          <input name="year" type="number" min={0} max={9999}/>
          <span>{"/"}</span>
          <input name="month" type="number" min={1} max={12} />
          <span>{"/"}</span>
          <input name="day" type="number" min={1} max={31} />
        </div>
        <span/>
        <div className={classes.delimiter}/>
        <p className={classes.explain}>
          {t({ id: "time.explain", mask: "Hour : Minute : Second" })}
        </p>
        <span/>
        <div className={classes.timeDisplay}>
          <input name="hour" type="number" min={0} max={23} />
          <span>{":"}</span>
          <input name="minute" type="number" min={0} max={59} />
          <span>{":"}</span>
          <input name="second" type="number" min={0} max={59} />
        </div>
        <button type="submit">{t({ id: "save" })}</button>
      </form>
    </section>
  );
};

export default SetTime;
