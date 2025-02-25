import subprocess
import os

def install_cuda():
    try:
        # Execute the apt-get command to install cudatoolkit
        command = ['wget', 'https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb']
        result = subprocess.run(command, check=True, text=True, capture_output=True)
        command = ['dpkg', '-i', 'cuda-keyring_1.1-1_all.deb']
        result = subprocess.run(command, check=True, text=True, capture_output=True)
        command = ['apt-get', 'update']
        result = subprocess.run(command, check=True, text=True, capture_output=True)
        command = ['apt-get', '-y', 'install', 'cuda-toolkit-12-6']
        result = subprocess.run(command, check=True, text=True, capture_output=True)

        # Print the output from the command
        print("Installation completed successfully.")
        print("Output:")
        print(result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred during installation:")
        print(e.stderr)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


def list_files_and_subfiles(directory="."):
    for root, dirs, files in os.walk(directory):
        print(f"Directory: {root}")
        for file in files:
            print(f"  File: {file}")
        for subdir in dirs:
            print(f"  Subdirectory: {subdir}")


def execute_install_script():
    try:
        # Ensure the script is executable
        script_path = './install.sh'
        if not os.access(script_path, os.X_OK):
            os.chmod(script_path, 0o755)

        # Run the script with the -g argument
        result = subprocess.run([script_path, '-g'], check=True, text=True)

        # Print the output from the script
        print("Script executed successfully.")
        print("Output:")
        print(result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred while executing the script:")
        print(e.stderr)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


def execute_parafrost():
    try:
        # Define the command and arguments
        command = ['./build/gpu/bin/parafrost', './cnf/simple.txt']

        # Ensure the file exists
        file_path = './cnf/simple.txt'
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"The file {file_path} does not exist.")

        # Run the command
        result = subprocess.run(command, check=True, text=True, capture_output=True)

        # Print the output from the command
        print("Command executed successfully.")
        print("Output:")
        print(result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred while executing the command:")
        print(e.stderr)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    install_cuda()
    execute_install_script()
    execute_parafrost()
