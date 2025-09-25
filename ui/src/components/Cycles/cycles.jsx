import React, { useCallback, useEffect, useState } from "react";
import { useI18nContext } from "../../contexts/i18nContext";
import { useDispatch, useStoreContext } from "../../contexts/storeContext";
import ListCycle from "./_listCycle";
import { STORE_ACTION_TYPES } from "../../contexts/actions";
import Dialog from "../Dialog";
import { CreateContent } from "./Actions";
import classes from "./cycles.module.css";

const Cycles = () => {
  const t = useI18nContext();
  const { cycles } = useStoreContext();
  const dispatch = useDispatch();
  const [open, setOpen] = useState(false);
  const handleClose = useCallback(() => {
    setOpen(false);
  }, []);

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
      <div className={classes.headingWrapper}>
        <h2 className={classes.heading}>
          {t({ id: "cycles", mask: "Cycles" })}
        </h2>
        <button
          className={classes.addBtn}
          onClick={() => setOpen(true)}
          type="button"
        >
          {"+"}
        </button>
      </div>
      <ListCycle data={cycles} />
      <Dialog
        className={classes.dialog}
        open={open}
        title={t({ id: "add.cycle", mask: "Add cycle" })}
        onClose={handleClose}
        content={<CreateContent {...{ onClose: handleClose }} />}
      />
    </section>
  );
};

export default Cycles;
