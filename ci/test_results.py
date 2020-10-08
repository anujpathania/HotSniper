import unittest

from simulationcontrol import resultlib

class SanityCheck(unittest.TestCase):
    def test_runs_present(self):
        runs = list(resultlib.get_runs())
        self.assertEqual(len(runs), 2)

    def test_performance(self):
        """
        test that performance can be parsed
        """
        for run in resultlib.get_runs():
            resp_time = resultlib.get_average_response_time(run)
            self.assertTrue(resp_time > 0)

    def test_power(self):
        """
        test that power traces have been created (don't test values)
        """
        for run in resultlib.get_runs():
            avgpower = resultlib.get_average_power_consumption(run)
            self.assertTrue(avgpower > 0)

    def test_temperature(self):
        """
        test that temperature traces have been created (don't test values)
        """
        for run in resultlib.get_runs():
            temp = resultlib.get_average_peak_temperature(run)
            self.assertTrue(temp > 45)


if __name__ == '__main__':
    unittest.main()
