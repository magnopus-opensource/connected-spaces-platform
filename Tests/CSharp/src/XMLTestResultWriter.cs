using System;
using System.IO;


namespace CSharpTests
{
    class XMLTestResultWriter : TestResultWriterBase
    {
        static double DurationToSeconds(long duration)
        {
            var seconds = duration / 1000.0;

            return Math.Round(seconds, 3);
        }

        readonly string filename;

        public XMLTestResultWriter(string filename)
        {
            this.filename = filename;
        }

        public override void WriteResults()
        {
            var file = File.CreateText(filename);
            file.WriteLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            file.WriteLine($"<testsuites tests=\"{ testCount }\" failures=\"{ failureCount }\" disabled=\"0\" errors=\"0\" time=\"{ DurationToSeconds(totalDuration) }\" timestamp=\"{ startTime:yyyy-MM-dd'T'HH:mm:ss.fffzzz}\" name=\"AllTests\">");

            foreach (var suitePair in testSuites)
            {
                var suite = suitePair.Value;

                file.WriteLine($"  <testsuite name=\"{ suite.Name }\" tests=\"{ suite.TestCount }\" failures=\"{ suite.FailureCount }\" disabled=\"0\" skipped=\"0\" errors=\"0\" time=\"{ DurationToSeconds(suite.Duration) }\" timestamp=\"{ suite.StartTime:yyyy-MM-dd'T'HH:mm:ss.fffzzz}\">");

                foreach (var testPair in suite.TestCases)
                {
                    var test = testPair.Value;

                    file.Write($"    <testcase name=\"{ test.Name }\" status=\"run\" result=\"completed\" time=\"{ DurationToSeconds(test.Duration) }\" timestamp=\"{ test.StartTime:yyyy-MM-dd'T'HH:mm:ss.fffzzz}\" classname=\"{ suite.Name }\"");

                    if (test.Passed)
                        file.WriteLine(" />");
                    else
                    {
                        file.WriteLine(">");
                        file.WriteLine("      <failure message=\"TODO: Print real error message.\" type=\"\"><![CDATA[TODO: Print real error message.]]></failure>");
                        file.WriteLine("    </testcase>");
                    }
                }

                file.WriteLine("  </testsuite>");
            }

            file.WriteLine("</testsuites>");
            file.Close();
        }
    }
}
