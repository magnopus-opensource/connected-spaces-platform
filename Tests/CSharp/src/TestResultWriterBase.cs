using System;
using System.Collections.Generic;


namespace CSharpTests
{
    abstract class TestResultWriterBase
    {
        protected class TestSuite
        {
            public string Name;
            public DateTime StartTime;
            public long Duration;
            public Dictionary<string, TestCase> TestCases;
            public int FailureCount;
            public int TestCount => TestCases.Count;
        }


        protected class TestCase
        {
            public string Name;
            public DateTime StartTime;
            public long Duration;
            public bool Passed;
        }


        protected int testCount;
        protected int failureCount;
        protected long totalDuration;
        protected DateTime startTime;

        protected readonly Dictionary<string, TestSuite> testSuites;

        public TestResultWriterBase()
        {
            testSuites = new Dictionary<string, TestSuite>();
        }

        public void AddTestSuite(string name)
        {
            testSuites.Add(name, new TestSuite
            {
                Name = name,
                TestCases = new Dictionary<string, TestCase>()
            });
        }

        public void AddTestCase(string suiteName, string testName, DateTime startTime, long duration, bool passed)
        {
            if (this.startTime == default)
                this.startTime = startTime;

            var suite = testSuites[suiteName];

            if (suite.TestCount == 0)
                suite.StartTime = startTime;

            suite.TestCases.Add(testName, new TestCase
            {
                Name = testName,
                StartTime = startTime,
                Duration = duration,
                Passed = passed
            });

            suite.Duration += duration;
            totalDuration += duration;

            testCount++;

            if (!passed)
            {
                suite.FailureCount++;
                failureCount++;
            }
        }

        public abstract void WriteResults();
    }
}
