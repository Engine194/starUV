import React, { Fragment, useCallback } from "react";
import logo from "../../logo.png";
import classes from "./app.module.css";
import { useI18nContext } from "../../contexts/i18nContext";
import { useDispatch, useStoreContext } from "../../contexts/storeContext";
import { classNames } from "../../utils";
import { STORE_ACTION_TYPES } from "../../contexts/actions";
import SetTime from "../SetTime";
import Cycles from "../Cycles";

function App() {
  const t = useI18nContext();
  const { language } = useStoreContext();
  const dispatch = useDispatch();
  const handleChangeLanguage = useCallback(
    (value) => {
      dispatch({
        type: STORE_ACTION_TYPES.UPDATE_LANGUAGE,
        payload: value.toLowerCase(),
      });
    },
    [dispatch]
  );
  return (
    <>
      <header className={classes.header}>
        <div className={classes.container}>
          <div className={classes.logoWrapper}>
            <img
            width={48}
            height={48}
            src={logo}
            className={classes.logo}
            alt="logo"
          />
          <strong className={classes.logoTitle}>{t({id: "logo", mask: "STARUV"})}</strong>
          </div>
          <div className={classes.languages}>
            {["EN", "VI"].map((item, index) => {
              return (
                <Fragment key={item}>
                  <button
                    type="button"
                    className={classNames(
                      classes.languageItem,
                      item.toLowerCase() === language && classes.active
                    )}
                    onClick={() => handleChangeLanguage(item)}
                  >
                    {item}
                  </button>
                  {index === 0 ? <span>{"|"}</span> : null}
                </Fragment>
              );
            })}
          </div>
        </div>
      </header>
      <div className={classes.container}>
        <h1 className={classes.heading}>
          {t({ id: "staruv.dashboard", mask: "StarUV Dashboard" })}
        </h1>
        <main>
          <SetTime />
          <Cycles />
        </main>
        <footer>
          <div>
            <p></p>
          </div>
        </footer>
      </div>
    </>
  );
}

export default App;
