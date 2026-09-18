import asyncio
import json
import math
import unittest
from contextlib import asynccontextmanager
from types import SimpleNamespace

from backend.bridge import Bridge, MESSAGES, UserError, coordinates, error_code


class FakeHardware:
    def __init__(self):
        self.present = True
        self.developer = True
        self.fail_mount = False
        self.fail_clear = False
        self.actions = []
        self.fake_serial = "PRIVATE-DEVICE-IDENTIFIER"

    async def list(self):
        return [SimpleNamespace(serial=self.fake_serial)] if self.present else []

    @asynccontextmanager
    async def connect(self, serial):
        async def mode():
            return self.developer
        yield SimpleNamespace(product_type="iPhone18,1", product_version="27.0", get_developer_mode_status=mode)

    @asynccontextmanager
    async def location(self, serial, progress):
        self.actions.append(("connect", serial))
        if self.fail_mount:
            raise UserError("mount")
        if not self.developer:
            raise UserError("developer")
        try:
            yield self
        finally:
            self.actions.append(("close",))

    async def set(self, lat, lon):
        self.actions.append(("set", lat, lon))

    async def clear(self):
        self.actions.append(("clear",))
        if self.fail_clear:
            raise RuntimeError(self.fake_serial)

    async def enable(self, serial):
        self.actions.append(("enable", serial))
        return "restart_required"


class CoordinateTests(unittest.TestCase):
    def test_valid_boundaries_and_negative_longitude(self):
        for a, b in [(90,180),(-90,-180),(0,0),(37.334643,-122.008972)]:
            self.assertEqual(coordinates(str(a), str(b)), (a,b))

    def test_nonfinite_and_invalid_rejected(self):
        for a, b in [(91,0),(0,181),(math.nan,0),(0,math.inf),(True,1),(None,1),("",0),("1; run",0)]:
            with self.subTest(lat=a,lon=b), self.assertRaises(UserError):
                coordinates(a,b)

    def test_raw_exception_is_not_exposed(self):
        self.assertEqual(error_code(RuntimeError("SECRET SERIAL: PRIVATE")), "unexpected")
        self.assertNotIn("PRIVATE", json.dumps(MESSAGES))


class BridgeTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.events = []
        self.hardware = FakeHardware()
        self.bridge = Bridge(self.events.append,self.hardware)
        await self.bridge.scan()
        self.token = next(iter(self.bridge.devices))

    async def asyncTearDown(self):
        await self.bridge.dispatch({"action":"quit"})

    async def wait_active(self):
        for _ in range(100):
            if any(e.get("state")=="active" for e in self.events):
                return
            await asyncio.sleep(.001)
        self.fail("session never reached active")

    def command(self, action, **extra):
        return {"action":action,"device":self.token,**extra}

    async def test_scan_sends_only_anonymous_device_details(self):
        self.assertNotIn(self.hardware.fake_serial,json.dumps(self.events))
        self.assertEqual(self.events[0]["devices"][0]["label"],"iPhone 1 · iOS 27.0")

    async def test_set_stop_clear_then_close(self):
        await self.bridge.dispatch(self.command("apply",lat=37.3,lon=-122))
        await self.wait_active()
        await self.bridge.dispatch({"action":"stop"})
        await self.bridge.task
        self.assertEqual([a[0] for a in self.hardware.actions],["connect","set","clear","close"])
        self.assertIn("restored",[e.get("state") for e in self.events])

    async def test_mount_failure_never_sets_or_claims_success(self):
        self.hardware.fail_mount = True
        await self.bridge.execute(self.command("apply",lat=0,lon=0))
        self.assertNotIn("set",[a[0] for a in self.hardware.actions])
        self.assertNotIn("active",[e.get("state") for e in self.events])
        self.assertIn("mount",[e.get("code") for e in self.events])

    async def test_clear_failure_does_not_claim_restored(self):
        self.hardware.fail_clear = True
        await self.bridge.dispatch(self.command("apply",lat=0,lon=0))
        await self.wait_active()
        await self.bridge.dispatch({"action":"stop"})
        await self.bridge.task
        self.assertIn("restore",[e.get("code") for e in self.events])
        self.assertNotIn("restored",[e.get("state") for e in self.events])
        self.assertNotIn(self.hardware.fake_serial,json.dumps(self.events))

    async def test_eof_quit_restores_before_exit(self):
        await self.bridge.dispatch(self.command("apply",lat=0,lon=0))
        await self.wait_active()
        await self.bridge.dispatch({"action":"quit"})
        self.assertEqual([a[0] for a in self.hardware.actions][-2:],["clear","close"])

    async def test_overlapping_commands_are_rejected(self):
        await self.bridge.dispatch(self.command("apply",lat=0,lon=0))
        await self.wait_active()
        await self.bridge.dispatch({"action":"scan"})
        self.assertIn("busy",[e.get("code") for e in self.events])
        self.assertIn(self.token,self.bridge.devices)

    async def test_scan_invalidates_old_selection(self):
        await self.bridge.scan()
        await self.bridge.execute(self.command("clear"))
        self.assertIn("device",[e.get("code") for e in self.events])
        self.assertEqual(self.hardware.actions,[])

    async def test_invalid_coordinates_never_open_device(self):
        await self.bridge.execute(self.command("apply",lat="nan",lon=0))
        self.assertEqual(self.hardware.actions,[])

    async def test_developer_enable_reports_restart_not_completion(self):
        await self.bridge.execute(self.command("enable_developer"))
        self.assertIn("restart_required",[e.get("state") for e in self.events])

    async def test_unplugged_device_does_not_claim_active_forever(self):
        await self.bridge.dispatch(self.command("apply",lat=0,lon=0))
        await self.wait_active()
        self.hardware.present=False
        await asyncio.wait_for(self.bridge.task,3)
        self.assertIn("device",[e.get("code") for e in self.events])
        self.assertIn(("clear",),self.hardware.actions)

    async def test_invalid_protocol(self):
        for command in ([],None,{"action":3},{}):
            await self.bridge.dispatch(command)
        self.assertEqual(len([e for e in self.events if e.get("code")=="protocol"]),4)

    async def test_locate_action(self):
        loc = await self.bridge.locate()
        self.assertIn("ok", loc)
        await self.bridge.execute({"action": "locate"})
        self.assertTrue(any(e.get("event") == "location" for e in self.events))


if __name__ == "__main__":
    unittest.main()
