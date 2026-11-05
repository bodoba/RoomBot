"""Data model for one sensor snapshot sent by the daemon."""

import json
from dataclasses import dataclass


@dataclass
class SensorState:
    """One decoded 'roombot/sensor/state' message.

    The field names follow the JSON keys written by toJson() in sensors.c.
    """

    seq: int = 0              # message counter, used to detect lost messages
    t_ms: int = 0             # daemon timestamp in milliseconds
    mode: int = -1            # OI mode: 0=Off 1=Passive 2=Safe 3=Full
    bump_raw: int = 0         # OI packet 7: bumps and wheel drops (bit field)
    wall: bool = False        # wall sensor
    cliff: tuple = (0, 0, 0, 0)  # left, front-left, front-right, right
    voltage_mv: int = 0
    current_ma: int = 0
    temp_c: int = 0
    charge_mah: int = 0
    capacity_mah: int = 0

    # --- Convenience properties: decode the bit field of packet 7 ---
    @property
    def bump_right(self) -> bool:
        return bool(self.bump_raw & 0x01)

    @property
    def bump_left(self) -> bool:
        return bool(self.bump_raw & 0x02)

    @property
    def wheeldrop_right(self) -> bool:
        return bool(self.bump_raw & 0x04)

    @property
    def wheeldrop_left(self) -> bool:
        return bool(self.bump_raw & 0x08)

    @property
    def battery_percent(self) -> float:
        """Battery charge in percent (0..100), 0 if capacity is unknown."""
        if self.capacity_mah <= 0:
            return 0.0
        return max(0.0, min(100.0, 100.0 * self.charge_mah / self.capacity_mah))

    @property
    def mode_name(self) -> str:
        return {0: "OFF", 1: "PASSIVE", 2: "SAFE", 3: "FULL"}.get(self.mode, "?")

    @classmethod
    def from_json(cls, text: str) -> "SensorState":
        """Parse the JSON text of a state message.

        Raises ValueError if the text is not valid or keys are missing.
        """
        try:
            d = json.loads(text)
            batt = d["batt"]
            return cls(
                seq=int(d["seq"]),
                t_ms=int(d["t"]),
                mode=int(d["mode"]),
                bump_raw=int(d["bump"]),
                wall=bool(d["wall"]),
                cliff=tuple(int(c) for c in d["cliff"]),
                voltage_mv=int(batt["mV"]),
                current_ma=int(batt["mA"]),
                temp_c=int(batt["C"]),
                charge_mah=int(batt["mAh"]),
                capacity_mah=int(batt["cap"]),
            )
        except (KeyError, TypeError, json.JSONDecodeError) as exc:
            # Convert all parsing problems into one exception type.
            raise ValueError(f"bad state message: {exc}") from exc