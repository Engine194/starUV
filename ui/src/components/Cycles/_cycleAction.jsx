import React, { useCallback, useEffect, useState } from "react";
import { EllipsisVertical } from "../Icons";
import { classNames } from "../../utils";
import classes from "./_cycleAction.module.css";
import { useI18nContext } from "../../contexts/i18nContext";
import Dialog from "../Dialog";
import { UpdateContent } from "./Actions";

const CycleAction = (props) => {
  const { className = "", selectState, data } = props || {};
  const t = useI18nContext();
  const [id, setId] = selectState;
  const active = id === data.id;
  const [openUpdate, setOpenUpdate] = useState(false);
  const [openDelete, setOpenDelete] = useState(false);
  const handleClose = useCallback(() => {
    setOpenUpdate(false);
    setId("");
  }, [setId]);

  useEffect(() => {
    const controller = new AbortController();
    window.addEventListener(
      "click",
      () => {
        setId("");
      },
      { signal: controller.signal }
    );

    return () => {
      controller.abort();
    };
  }, [setId]);

  return (
    <div
      onClick={(event) => event.stopPropagation()}
      className={classNames(classes.root, className)}
    >
      <button
        type="button"
        className={classNames(classes.mainBtn, active && classes.active)}
        onClick={() => setId(data.id)}
      >
        <EllipsisVertical />
      </button>
      {active && !openUpdate && !openDelete ? (
        <div className={classes.dropdownAction}>
          <ul className={classes.menu}>
            <li>
              <button
                className={classes.actionBtn}
                type="button"
                onClick={() => setOpenUpdate(true)}
              >
                {t({ id: "update" })}
              </button>
            </li>
            <li>
              <button
                className={classes.actionBtn}
                type="button"
                onClick={() => setOpenDelete(true)}
              >
                {t({ id: "delete" })}
              </button>
            </li>
          </ul>
        </div>
      ) : null}

      <Dialog
        className={classes.dialog}
        open={openUpdate}
        title={t({ id: "update.cycle", mask: "Update cycle" })}
        onClose={handleClose}
        content={<UpdateContent {...{ data, onClose: handleClose }} />}
      />
    </div>
  );
};

export default CycleAction;
