from tkinter import *
import shutil
import stat
import os
import json
from tkinter import filedialog
from tkinter import messagebox

configs = [
    "DebugDLL",
    # "DebugStatic",  # TODO: Add Static lib support
    "ReleaseDLL",
    # "ReleaseStatic"  # TODO: Add Static lib support
]

platforms = [
    'Android',
    # 'IOS',  # TODO: Add IOS Support
    # 'Mac',  # TODO: Add Mac support
    'Win64'
]

# We use this map to get the path conversion for foundation from client
foundation_platforms = {
    'Android': 'Android',
    'IOS': 'ios',
    'Mac': 'macosx',
    'Win64': 'x64'
}

# Application colors
color_bg = '#131631'
color_button = '#9b51e0'
color_button_active = '#573388'

# Application icons
icon_folder = ""

# Globals
oko_foundation_path = ""
oko_client_path = ""
saved_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'Saved'))
binaries_relative_foundation_path = "/Library/Binaries"
include_relative_foundation_path = "/Library/include"
binaries_relative_client_path = "/Plugins/OKO/Binaries/ThirdParty/CSP"
include_relative_client_path = "/Plugins/OKO/Source/ThirdParty"
saved_application_data = json.loads("{}")


# Save/load methods
def save_config(data):
    global saved_path
    with open(saved_path + '/config.json', 'w') as out_file:
        json.dump(data, out_file, indent=4)


def load_config():
    global saved_application_data
    global saved_path
    with open(saved_path + '/config.json') as json_file:
        saved_application_data = json.load(json_file)


# Directory utilities
def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)


def copy_directory_contents(source_dir, target_dir):
    copytree(source_dir, target_dir)


def force_remove_directory_contents(in_dir):
    for root, dirs, files in os.walk(in_dir, topdown=False):
        for name in files:
            filename = os.path.join(root, name)
            os.chmod(filename, stat.S_IWUSR)
            os.remove(filename)
        for name in dirs:
            os.rmdir(os.path.join(root, name))


def show_error(error_message):
    messagebox.showerror("Error", error_message)


# Widget for selecting a path
class PathBrowser:
    def __init__(self, in_id, master, label_text):
        global saved_application_data
        self.id = in_id
        self.path = StringVar()
        self.entry = ""

        frame = Frame(master, bg=master['bg'])
        frame.pack(fill=X, expand=True, anchor=N, side=TOP, padx=10, pady=10)

        label = Label(frame, text=label_text, bg=master['bg'], fg='white')
        label.pack(side=TOP, anchor=W, padx=5, pady=5)

        self.path.trace_add("write", self.on_modified)
        self.entry = Entry(frame, relief=FLAT, textvariable=self.path)
        self.entry.pack(fill=X, padx=5, pady=5, expand=True, side=LEFT)

        browse_button = Button(frame, bg=master['bg'], bd=0, image=icon_folder, relief=FLAT, command=self.select_path)
        browse_button.pack(side=LEFT)

        # Check if we have save data for this object
        if self.id in saved_application_data:
            self.path.set(saved_application_data[self.id])
            self.update_entry()

    def on_modified(self, var, index, mode):
        self.save_path()

    def select_path(self):
        global saved_application_data
        self.path.set(os.path.normpath(filedialog.askdirectory()))
        self.save_path()
        self.update_entry()

    def update_entry(self):
        if self.path.get() != "":
            entry_text = self.path.get()
            self.entry.delete(0, END)
            self.entry.insert(0, entry_text)

    def save_path(self):
        saved_application_data.update({
            self.id: self.path.get()
        })
        save_config(saved_application_data)


class CopyFoundationToClientButton:
    def __init__(self, in_oko_build_tool, master):
        self.oko_build_tool = in_oko_build_tool
        global color_button
        global color_button_active

        button = Button(
            master,
            bg=color_button,
            activeforeground='white',
            activebackground=color_button_active,
            bd=0,
            text="COPY CSP LIBS TO CLIENT",
            fg='white',
            font='Helvetica 8 bold',
            command=self.copy_foundation_to_client,
            padx=5,
            pady=5,
            relief=FLAT,
            width=50,
            cursor="right_side"
        )
        button.pack()

    def copy_foundation_to_client(self):
        global binaries_relative_client_path
        global include_relative_client_path
        global binaries_relative_foundation_path
        global include_relative_foundation_path
        global foundation_platforms

        # Handle user error
        if not os.path.isdir(self.oko_build_tool.foundation_path.path.get()):
            show_error("Please provide a valid path for your CSP workspace")
            return
        elif not os.path.isdir(self.oko_build_tool.client_path.path.get()):
            show_error("Please provide a valid path for your OKO Client workspace")
            return

        chosen_platforms = self.oko_build_tool.platforms.get_checked_buttons()
        if len(chosen_platforms) <= 0:
            show_error("You must choose at least one platform")
            return

        chosen_configs = self.oko_build_tool.configs.get_checked_buttons()
        if len(chosen_configs) <= 0:
            show_error("You must choose at least one config")
            return

        # These arrays should be in sync that copy_from should to copy_to of the same index
        paths_to_copy_from = []
        paths_to_copy_to = []
        # Error handling..
        non_existent_paths = []
        empty_paths = []

        # Get all of the paths we need to copy from and to
        for platform in chosen_platforms:
            platform = platform.option

            binaries_client_path = self.oko_build_tool.client_path.path.get() + \
                                   binaries_relative_client_path

            binaries_client_path += "/" + platform
            if platform == 'Android':  # @RossB: Edge case handling for putting Android into nested arm64-v8a dir.
                binaries_client_path += "/" + "arm64-v8a"
                binaries_foundation_path = self.oko_build_tool.foundation_path.path.get() + \
                                           "/ARM64"
            else:
                binaries_foundation_path = self.oko_build_tool.foundation_path.path.get() + \
                                           binaries_relative_foundation_path
                binaries_foundation_path += "/" + foundation_platforms[platform]

            for config in chosen_configs:
                config = config.option
                config_client_path = binaries_client_path

                if platform == 'Android':
                    config_foundation_path = binaries_foundation_path + "/" + config + " Android/"
                else:
                    config_foundation_path = binaries_foundation_path + "/" + config

                config_foundation_path = os.path.normpath(config_foundation_path)
                if os.path.isdir(config_foundation_path):
                    if len(os.listdir(config_foundation_path)) == 0:
                        empty_paths.append(config_foundation_path)
                    else:
                        paths_to_copy_from.append(config_foundation_path)
                else:
                    non_existent_paths.append(config_foundation_path)

                # @RossB: Although we don't strictly need the target directory to pre-exist, it is more than likely
                # to pre-exist and so we can use this as a means of validating the user's client path is correct
                config_client_path = os.path.normpath(config_client_path)
                if os.path.isdir(config_client_path):
                    paths_to_copy_to.append(config_client_path)
                else:
                    non_existent_paths.append(config_client_path)

        # Early out if the paths we are attempting to copy from are invalid or do not have any content
        for non_existent_path in non_existent_paths:
            show_error("The path " + non_existent_path + " does not exist!")
            return

        for empty_path in empty_paths:
            show_error("There is no content to copy in " + empty_path)
            return

        include_foundation_path = self.oko_build_tool.foundation_path.path.get() + include_relative_foundation_path

        # Early out if we have no include content to copy
        if os.path.isdir(include_foundation_path):
            if len(os.listdir(include_foundation_path)) == 0:
                show_error("There is no content to copy in " + include_foundation_path)
                return
        else:
            show_error("The path " + include_foundation_path + " does not exist!")
            return

        # No we have checked we have everything we need to copy over, we can start deleting the old binaries/includes
        for path in paths_to_copy_to:
            self.oko_build_tool.status_bar.update_status_text("Deleting content in " + path)
            force_remove_directory_contents(path)

        include_client_path = self.oko_build_tool.client_path.path.get() + include_relative_client_path
        self.oko_build_tool.status_bar.update_status_text("Deleting content in " + include_client_path)
        shutil.rmtree(include_client_path + "/CSP", ignore_errors=False, onerror=None)

        # Begin copying over the libs
        for i in range(len(paths_to_copy_to)):
            path_to_copy_from = paths_to_copy_from[i]
            path_to_copy_to = paths_to_copy_to[i]
            self.oko_build_tool.status_bar.update_status_text(
                "Copying contents from " + path_to_copy_from + " to " + path_to_copy_to)
            copy_directory_contents(path_to_copy_from, path_to_copy_to)

        # Finally, copy over the includes
        self.oko_build_tool.status_bar.update_status_text("Copying includes..")
        copy_directory_contents(include_foundation_path, include_client_path)

        self.oko_build_tool.status_bar.update_status_text("Done!")


# Simple widget to report error messages / status updates
class StatusBar:
    def __init__(self, master):
        self.label = ""

        self.label = Label(master, text="..", anchor=W, bg='black', fg='grey')
        self.label.pack(side=BOTTOM, fill=X)

    def update_status_text(self, text):
        self.label.config(text=text)


# Check Button class for use with platforms and configs
class MultiOptionCheckButton:
    def __init__(self, master, option):
        global saved_application_data
        self.option = option
        self.button_state = IntVar()

        # Check if we have save data for this check button
        if self.option in saved_application_data:
            self.button_state.set(saved_application_data[self.option])

        option_check_button = Checkbutton(
            master, text=option, width=20, bg=master['bg'], fg='white', selectcolor='black',
            activebackground=color_bg, activeforeground='white', relief='flat', anchor='w',
            variable=self.button_state, command=self.on_state_changed
        )
        option_check_button.pack(fill=X, expand=True, anchor=W, side=TOP, padx=10, pady=0)

    def on_state_changed(self):
        global saved_application_data
        saved_application_data.update({
            self.option: self.get_value()
        })
        save_config(saved_application_data)

    def get_value(self):
        return self.button_state.get()


# Wrapper class for multiple check buttons
class MultiOptionSelect:
    def __init__(self, master, options):
        self.check_buttons = []
        for i in range(len(options)):
            option = options[i]
            self.check_buttons.append(MultiOptionCheckButton(master, option))

    # Helper function to get check buttons whose state is current set to checked
    def get_checked_buttons(self):
        checked_buttons = []
        for option in self.check_buttons:
            if option.get_value() == 1:
                checked_buttons.append(option)
        return checked_buttons


# Main UI window
class OKOBuildTool(Frame):
    def __init__(self):
        super().__init__()
        self.foundation_path = ""
        self.client_path = ""
        self.platforms = ""
        self.configs = ""
        self.status_bar = ""

        self.init_ui()

    def init_ui(self):
        global platforms
        self.master.title("OKO Build Tool")

        # Build out the UI
        path_browsers_frame = Frame(self.master, bg=self.master['bg'])
        path_browsers_frame.pack(fill=X, expand=True, anchor=N, side=TOP, padx=10, pady=10)

        Label(path_browsers_frame, text="Workspace paths:", bg=self.master['bg'], fg='white', font='Helvetica 8 bold',)\
            .pack(anchor='w')
        # Create both directory browsers
        self.foundation_path = PathBrowser(
            "foundation_path", path_browsers_frame, "CSP workspace path:")
        self.client_path = PathBrowser(
            "client_path", path_browsers_frame, "OKO Client workspace path:")

        multi_select_menu_frame = Frame(self.master, bg=self.master['bg'])
        multi_select_menu_frame.pack(fill=X, expand=True, anchor=N, side=TOP, padx=10, pady=0)

        platforms_frame = Frame(multi_select_menu_frame, bg=self.master['bg'])
        Label(platforms_frame, text="Platforms:", bg=self.master['bg'], fg='white', font='Helvetica 8 bold',)\
            .pack(anchor='w')
        self.platforms = MultiOptionSelect(platforms_frame, platforms)

        configs_frame = Frame(multi_select_menu_frame, bg=self.master['bg'])
        Label(configs_frame, text="Configs:", bg=self.master['bg'], fg='white', font='Helvetica 8 bold',)\
            .pack(anchor='w')
        self.configs = MultiOptionSelect(configs_frame, configs)

        platforms_frame.pack(fill=X, expand=True, anchor=N, side=LEFT, padx=10, pady=0)
        configs_frame.pack(fill=X, expand=True, anchor=N, side=LEFT, padx=10, pady=0)

        transfer_frame = Frame(self.master, bg=self.master['bg'])
        CopyFoundationToClientButton(self, transfer_frame)
        transfer_frame.pack(fill=X, expand=True, anchor=N, side=TOP, padx=10, pady=0)

        self.status_bar = StatusBar(self.master)


def main():
    global icon_folder
    global saved_path

    os.makedirs(saved_path, exist_ok=True)  # Make config directory if it does not already exist

    if not os.path.isfile(saved_path + '/config.json'):  # Make default config.json file if it does not already exist
        empty_json = {}
        save_config(empty_json)
    else:
        load_config()

    root = Tk()
    root.iconbitmap('../Images/oko_icon.ico')

    # Background Styling
    background_image = PhotoImage(file="../Images/OKOBuildTool_Logo.png")
    background_image_label = Label(
        root,
        image=background_image,
        bd=0
    )

    background_image_label['bg'] = background_image_label.master['bg']
    background_image_label.pack()

    # Initialise icons
    icon_folder = PhotoImage(file="../Images/Icon_Folder.png")

    root.geometry("366x490")
    root.resizable(False, False)
    root.configure(background=color_bg)
    OKOBuildTool()  # Start the tool..
    root.mainloop()


if __name__ == '__main__':
    main()
