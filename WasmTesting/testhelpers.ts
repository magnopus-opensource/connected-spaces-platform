import { v4 as uuidv4 } from 'uuid';
import { Systems, Common } from 'connected-spaces-platform.web';
import * as assert from 'uvu/assert';
import puppeteer, { LaunchOptions } from 'puppeteer';

export const TEST_ACCOUNT_PASSWORD = "3R{d2}3C<x[J7=jU";

export async function CreateTestUser(): Promise<Systems.ProfileResult> {
  const userSystem = Systems.SystemsManager.get().getUserSystem();

  const uniqueUserName = "CSP-TEST-NAME" + uuidv4();
  const testDisplayName = "CSP-TEST-DISPLAY";
  const uniqueEmail = "testnopus.pokemon+" + uuidv4() + "@magnopus.com";

  const user = await userSystem.createUser(
    uniqueUserName,                         // userName
    testDisplayName,                        // displayName
    uniqueEmail,                            // email
    TEST_ACCOUNT_PASSWORD,                  // password
    false,                                  // receiveNewsletter
    true,                                   // userHasVerifiedAge
    null,                                   // redirectUrl
    null,                                   // inviteToken
  );

  assert.is(user.getHttpResultCode(), 201, "Creating temporary test user failed, got non-success return code.");
  return user;
}

export async function LoginAsUser(creds: Systems.ProfileResult){
  const userSystem = Systems.SystemsManager.get().getUserSystem();

  // Logs or errors are validated in the invoking test.
  const loginResult = await userSystem.login('', creds.getProfile().email, TEST_ACCOUNT_PASSWORD, true);
  if (loginResult.getResultCode() == Systems.EResultCode.Success){
    console.log("Successfully logged in");
  }
  else {
    console.error("Failed to log in");
    throw new Error("Failed to login");
  }

  loginResult.delete();
}

export async function LogoutUser(creds: Systems.ProfileResult){
  const userSystem = Systems.SystemsManager.get().getUserSystem();

  // Logs or errors are validated in the invoking test.
  const loginResult = await userSystem.logout();
  if (loginResult.getResultCode() == Systems.EResultCode.Success){
    console.log("Successfully logged out");
  }
  else {
    console.warn("Failed to log out");
  }

  loginResult.delete();
}

export async function CreatePublicTestSpace(): Promise<string> {
  const spaceSystem = Systems.SystemsManager.get().getSpaceSystem();

  let metadata = Common.Map.ofStringAndString();
  metadata.set("site", "Void");

  const spaceResult = await spaceSystem.createSpace(
    uuidv4(),
    "This is a wasm test space",
    Systems.SpaceAttributes.Public,
    null,
    metadata,
    null,
    null
  );
  console.log(spaceResult.getSpace().id);

  if (spaceResult.getResultCode() === Systems.EResultCode.Success) {
    return spaceResult.getSpace().id;
  }
  spaceResult.delete();
  throw new Error("Failed to create space");
}

export async function DeleteSpace(spaceId : string) : Promise<void> {
  const spaceSystem = Systems.SystemsManager.get().getSpaceSystem();

  let spaceDeleteResult =  await spaceSystem.deleteSpace(spaceId);

  if (spaceDeleteResult.getResultCode() === Systems.EResultCode.Success) {
    console.log("Succesfully deleted space.");
  }
  else {
    console.warn("Failed to delete space");
  }
  spaceDeleteResult.delete();
}

export interface PageTestResult {
    errors: Error[];
    consoleMessages: string[];
  }

export interface UserCreds {
    email: string;
    password: string;
}
  
  //Generic function to launch puppeteer, run a specified HTML file, and yield back state to be asserted against.
export async function LaunchTestPage(
    htmlPath: string,
    useDebugCSP: boolean,
    userCredentials: UserCreds | null,
    spaceId : string | null,
    launchOptions: LaunchOptions = {
      headless: true,
      args: [
        '--disable-application-cache',
        '--disable-cache', //Disabling cache just to be safe with repeated runs.
        '--no-sandbox',
        '--disable-setuid-sandbox' //Disabling sandbox as CI runners sometimes disable kernel features that sandboxing needs.
      ],
    },
    waitUntil: Array<'load' | 'domcontentloaded' | 'networkidle0' | 'networkidle2'> = ['load', 'networkidle0'], //Default will almost always be fine
  ): Promise<PageTestResult> {
    const browser = await puppeteer.launch(launchOptions);
    const page = await browser.newPage();
    const errors: Error[] = [];
    const consoleMessages: string[] = [];
  
    page.on('pageerror', e => errors.push(e));
    page.on('console', msg => consoleMessages.push(msg.text()));

    //Build a URL argument list.
    //This is a bit rough and ready, since credentials and spaceID are both optional and we need to handle the formatting
    //There's almost certainly a better way to do this, probably even a native JS way specifically for URL arguments.
    var args = `?useDebugCsp=${encodeURIComponent(useDebugCSP)}`;

    if (userCredentials !== null && userCredentials.email && userCredentials.password) {
      args = `${args}&email=${encodeURIComponent(userCredentials.email)}&password=${encodeURIComponent(userCredentials.password)}`;
    }

    if (spaceId != null) {
      args = `${args}&spaceId=${encodeURIComponent(spaceId)}`;
    }

    // Append the arguments to the HTML file path
    htmlPath = `${htmlPath}${args}`;
 
    console.log('Loading test page: ', htmlPath)
    
    await page.goto(htmlPath, { waitUntil });

    // Annoyingly, networkidle0 doesn't account for everything (I suspect it doesn't know how to validate connections originating from WASM, so give ourselves 5 seconds extra here).
    await new Promise(resolve => setTimeout(resolve, 5000));

    await page.close();
    await browser.close();
    return { errors, consoleMessages };
}