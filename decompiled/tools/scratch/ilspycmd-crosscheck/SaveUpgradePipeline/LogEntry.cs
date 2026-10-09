namespace SaveUpgradePipeline
{
	public class LogEntry
	{
		public TestResult testResult;

		public bool upgraded;

		public LogEntry(TestResult testResult, bool upgraded)
		{
			this.testResult = testResult;
			this.upgraded = upgraded;
		}

		public override string ToString()
		{
			switch (testResult)
			{
			case TestResult.Failed:
				return string.Concat("<color=red>", testResult, "</color> \t\t| ", upgraded ? "Upgraded" : "Not Upgraded");
			case TestResult.TooEarly:
				return string.Concat("<color=yellow>", testResult, "</color> \t\t| ", upgraded ? "Upgraded" : "Not Upgraded");
			default:
				return string.Concat(testResult, " \t| ", upgraded ? "<color=green>Upgraded</color>" : "<color=yellow>Not Upgraded</color>");
			case TestResult.Pass:
				return string.Concat("<color=green>", testResult, "</color> \t\t| ", upgraded ? "<color=green>Upgraded</color>" : "Not Upgraded");
			}
		}
	}
}
