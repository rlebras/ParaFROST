import subprocess
import os

def execute_install_script():
    try:
        # Ensure the script is executable
        script_path = './install.sh'
        if not os.access(script_path, os.X_OK):
            os.chmod(script_path, 0o755)

        # Run the script with the -g argument
        result = subprocess.run([script_path, '-g'], check=True, text=True, capture_output=True)

        # Print the output from the script
        print("Script executed successfully.")
        print("Output:")
        print(result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred while executing the script:")
        print(e.stderr)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    execute_install_script()
