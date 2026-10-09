using System.Collections.Generic;

namespace KSP.Testing
{
	public class TestResults
	{
		public int failed;

		public int success;

		public List<TestState> states = new List<TestState>();
	}
}
