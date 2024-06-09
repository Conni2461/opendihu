import .settings_muscle_contraction

settings_muscle_contraction.config["checkpointing"] = {
    "interval": 10,
    "directory": "states",
    "autoRestore": False,
}
