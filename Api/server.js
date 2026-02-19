const express = require("express");
const fs = require("fs");

const app = express();
app.use(express.json());

// Charge le mapping depuis le fichier JSON
const RFID_MAP = JSON.parse(fs.readFileSync("./rfid-map.json", "utf-8"));

const STORE_TO_WAREHOUSE = { A: 1, B: 2, C: 3 };
const STORE_TO_ANGLE = { A: 5, B: 15, C: 25 };

// Route de test
app.get("/health", (req, res) => {
  res.json({ status: "ok" });
});

// Route principale : renvoie la destination selon UID RFID
app.get("/api/routing/by-rfid/:uid", (req, res) => {
  const uid = String(req.params.uid || "").trim().toUpperCase();

  const store = RFID_MAP[uid];
  if (!store) {
    return res.status(404).json({
      error: "UNKNOWN_UID",
      uid,
      message: "UID RFID inconnu (non mappe)."
    });
  }

  return res.json({
    uid,
    store,
    warehouseId: STORE_TO_WAREHOUSE[store],
    servoAngle: STORE_TO_ANGLE[store]
  });
});

const PORT = 3000;
app.listen(PORT, "0.0.0.0", () => {
  console.log(`API running on http://localhost:${PORT}`);
});
