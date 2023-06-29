import { test, assert } from "../test_framework.js";
import { generateUniqueString } from "../test_helpers.js";
import { logIn } from "./usersystem_tests_helpers.js";
import { createSpace } from "./spacesystem_tests_helpers.js";

import { Systems } from "../olympus_foundation.js";

test("GraphQLSystemTests", "QueryTest", async function () {
  const systemsManager = Systems.SystemsManager.get();
  const userSystem = systemsManager.getUserSystem();
  const spaceSystem = systemsManager.getSpaceSystem();
  const GraphQLSystem = systemsManager.getGraphQLSystem();

  // Generate space name
  const spaceName = generateUniqueString("OLY-TESTS-WASM-SPACE");
  const spaceDescription = "OLY-TESTS-WASM-SPACEDESC";

  // Log in
  await logIn(userSystem);

  // Create space
  const space = await createSpace(
    spaceSystem,
    spaceName,
    spaceDescription,
    Systems.SpaceAttributes.Private
  );

  const query = "spaces{items{name}}";

  // Run GraphQL Query
  const result = await GraphQLSystem.runQuery(query);

  // Check response contain generated space name
  const response = result.getResponse();
  result.delete();

  assert.isTrue(response.includes(spaceName));

  const fullQuery = {
    query: `query getSpaceNameById($spaceId:String!)  {
              spaceDetails(groupId: $spaceId) {
                    name
                  }
                }`,
    variables: {
      spaceId: space.id,
    },
    operationName: "getSpaceNameById",
  };


  const fqResult = await GraphQLSystem.runQuery(JSON.stringify(fullQuery));
  const fqResponse = fqResult.getResponse();
  fqResult.delete();

  assert.isTrue(fqResponse.includes(spaceName));
});
