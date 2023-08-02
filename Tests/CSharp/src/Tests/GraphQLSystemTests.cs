using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

using Common = Csp.Common;
using Services = Csp.Services;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    static class GraphQLSystemTests
    {
#if RUN_ALL_UNIT_TESTS || RUN_GRAPHQLSYSTEM_TESTS || RUN_GRAPHQLSYSTEM_QUERY_TEST
        [Test]
        public static void QueryGraphQLTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out var graphQLSystem, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            _ = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Run Query
            var testQuery = "spaces{items{name}}";
            
            using var result = graphQLSystem.RunQuery(testQuery).Result;
            var test = result.GetResponse().ToString();

            // Check Response Contains Space Name
            Assert.IsTrue(test.Contains(testSpaceName));

            //Test our full query endpoint.
            var testFullQuery = "{\"query\":\"\n\nquery getSpaces($limit:Int!)  {\n  spaces(pagination: {limit:$limit}) {\n    items {\n      name\n    }\n  }\n}\n\n\",\"variables\":{\"limit\":5},\"operationName\":\"getSpaces\"}";

            using var resultFQ = graphQLSystem.RunRequest(testFullQuery).Result;
            var testFQ = resultFQ.GetResponse().ToString();

            // Check Response Contains Space Name
            Assert.IsTrue(testFQ.Contains(testSpaceName));

        }
#endif
    }
}
