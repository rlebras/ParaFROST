import subprocess
import os

def execute_parafrost():
    try:
        # Define the command and arguments
        command = ['parafrost', './cnf/simple.txt']

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
    execute_parafrost()
