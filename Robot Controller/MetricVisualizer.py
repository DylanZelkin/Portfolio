import tkinter as tk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time
import json
import os

class GraphWindow:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Model Performance Metrics")

        # Set the window size
        self.root.geometry("600x875")  # Increased height to fit 4 graphs

        # Create Frames
        self.frame1 = tk.Frame(self.root)
        self.frame1.pack(fill=tk.BOTH, expand=True)

        self.frame2 = tk.Frame(self.root)
        self.frame2.pack(fill=tk.BOTH, expand=True)

        # Create a figure with 4 subplots stacked vertically
        self.fig, (self.ax1, self.ax2, self.ax3, self.ax4) = plt.subplots(4, 1, figsize=(6, 12))  # 4 rows, 1 column

        # Create secondary y-axes for each graph
        self.ax1b = self.ax1.twinx()
        self.ax2b = self.ax2.twinx()
        self.ax3b = self.ax3.twinx()
        self.ax4b = self.ax4.twinx()

        # Set titles for each graph
        self.ax1.set_title("Reward and Entropy")
        self.ax2.set_title("Policy and Value Function Loss")
        self.ax3.set_title("KL Loss and VF Explained Variance")
        self.ax4.set_title("FPS and TODO")

        # Set shared x-axis label
        self.ax4.set_xlabel("Iteration")

        # Create a canvas for the Matplotlib figure and add it to Tkinter
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.frame1)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Section 2: Text display
        self.text = tk.Label(self.frame2, text="Performance Metrics Display", justify="left", anchor="w")
        self.text.pack(fill=tk.BOTH, expand=True)

        # Initialize data for the graphs
        self.x_data = []
        self.reward_mean = []
        self.episode_length = []
        self.policy_loss = []
        self.value_loss = []
        self.avg_action_time = []
        self.avg_time_between_actions = []
        self.learning_rate = []
        self.clip_fraction = []

        self.current_x = 1  # Start the x-axis at 1

    def update_graph1(self, reward_mean, episode_length):
        self.x_data.append(self.current_x)
        self.reward_mean.append(reward_mean)
        self.episode_length.append(episode_length)
        self.current_x += 1

        self.ax1.clear()
        self.ax1b.clear()

        self.ax1.plot(self.x_data, self.reward_mean, label="Reward Mean", color='green')
        self.ax1.set_ylabel("Reward Mean", color='green')
        self.ax1.tick_params(axis='y', labelcolor='green')

        self.ax1b.plot(self.x_data, self.episode_length, label="Entropy", color='blue')
        self.ax1b.set_ylabel("Entropy", color='blue')
        self.ax1b.tick_params(axis='y', labelcolor='blue')
        self.ax1b.yaxis.set_label_position('right')

        self.ax1.set_title("Reward and Episode Length")
        self.ax1.xaxis.set_major_locator(plt.MaxNLocator(integer=True))

        self.fig.tight_layout()
        self.canvas.draw()

    def update_graph2(self, policy_loss, value_loss):
        self.policy_loss.append(policy_loss)
        self.value_loss.append(value_loss)

        self.ax2.clear()
        self.ax2b.clear()

        self.ax2.plot(self.x_data, self.policy_loss, label="Policy Loss", color='red')
        self.ax2.set_ylabel("Policy Loss", color='red')
        self.ax2.tick_params(axis='y', labelcolor='red')

        self.ax2b.plot(self.x_data, self.value_loss, label="Value Loss", color='purple')
        self.ax2b.set_ylabel("Value Function Loss", color='purple')
        self.ax2b.tick_params(axis='y', labelcolor='purple')
        self.ax2b.yaxis.set_label_position('right')

        self.ax2.set_title("Policy and Value Function Loss")
        self.ax2.xaxis.set_major_locator(plt.MaxNLocator(integer=True))

        self.fig.tight_layout()
        self.canvas.draw()

    def update_graph3(self, avg_action_time, avg_time_between_actions):
        self.avg_action_time.append(avg_action_time)
        self.avg_time_between_actions.append(avg_time_between_actions)

        self.ax3.clear()
        self.ax3b.clear()

        self.ax3.plot(self.x_data, self.avg_action_time, label="Mean KL Loss", color='orange')
        self.ax3.set_ylabel("Mean KL Loss", color='orange')
        self.ax3.tick_params(axis='y', labelcolor='orange')

        self.ax3b.plot(self.x_data, self.avg_time_between_actions, label="VF Explained Variance", color='brown')
        self.ax3b.set_ylabel("VF Explained Variance", color='brown')
        self.ax3b.tick_params(axis='y', labelcolor='brown')
        self.ax3b.yaxis.set_label_position('right')

        self.ax3.set_title("Action Timing Metrics")
        self.ax3.xaxis.set_major_locator(plt.MaxNLocator(integer=True))

        self.fig.tight_layout()
        self.canvas.draw()

    def update_graph4(self, learning_rate, clip_fraction):
        self.learning_rate.append(learning_rate)
        self.clip_fraction.append(clip_fraction)

        self.ax4.clear()
        self.ax4b.clear()

        self.ax4.plot(self.x_data, self.learning_rate, label="FPS", color='darkred')
        self.ax4.set_ylabel("FPS", color='darkred')
        self.ax4.tick_params(axis='y', labelcolor='darkred')

        self.ax4b.plot(self.x_data, self.clip_fraction, label="TODO", color='teal')
        self.ax4b.set_ylabel("TODO", color='teal')
        self.ax4b.tick_params(axis='y', labelcolor='teal')
        self.ax4b.yaxis.set_label_position('right')

        self.ax4.set_title("FPS and TODO")
        self.ax4.xaxis.set_major_locator(plt.MaxNLocator(integer=True))

        self.fig.tight_layout()
        self.canvas.draw()

    # def save_graph(self, filepath="", filename="performance_graph.png"):
    #     try:
    #         self.fig.savefig(filepath + filename)
    #         print(f"Graph successfully saved as {filename}")
    #     except Exception as e:
    #         print(f"Error saving graph: {e}")

    def save_graph(self, filepath="", filename="performance_graph.png"):
        try:
            # Save the graph
            self.fig.savefig(filepath + filename)
            #print(f"Graph successfully saved as {filename}")

            # Save metrics to a JSON file
            metrics_data = {
                "x_data": self.x_data,
                "reward_mean": self.reward_mean,
                "episode_length": self.episode_length,
                "policy_loss": self.policy_loss,
                "value_loss": self.value_loss,
                "avg_action_time": self.avg_action_time,
                "avg_time_between_actions": self.avg_time_between_actions,
                "learning_rate": self.learning_rate,
                "clip_fraction": self.clip_fraction,
                "current_x": self.current_x
            }

            metrics_filename = filename.replace(".png", ".json")
            with open(filepath + metrics_filename, "w") as f:
                json.dump(metrics_data, f)
            
            #print(f"Metrics successfully saved as {metrics_filename}")

        except Exception as e:
            print(f"Error saving graph or metrics: {e}")

    def load_metrics(self, filepath="", filename="performance_graph.json"):
        try:
            full_path = os.path.join(filepath, filename)
            if os.path.exists(full_path):
                with open(full_path, "r") as f:
                    metrics_data = json.load(f)

                self.x_data = metrics_data["x_data"]
                self.reward_mean = metrics_data["reward_mean"]
                self.episode_length = metrics_data["episode_length"]
                self.policy_loss = metrics_data["policy_loss"]
                self.value_loss = metrics_data["value_loss"]
                self.avg_action_time = metrics_data["avg_action_time"]
                self.avg_time_between_actions = metrics_data["avg_time_between_actions"]
                self.learning_rate = metrics_data["learning_rate"]
                self.clip_fraction = metrics_data["clip_fraction"]
                self.current_x = metrics_data["current_x"]

                print(f"Metrics successfully loaded from {filename}")
            else:
                print(f"No saved metrics found at {full_path}")

        except Exception as e:
            print(f"Error loading metrics: {e}")

    def get_best_reward(self):
        """Retrieve the best (highest) reward from the loaded metrics."""
        if self.reward_mean:
            return max(self.reward_mean)
        else:
            print("No reward data available.")
            return None


    @classmethod
    def start(cls):
        def run():
            cls.instance = cls()
            cls.instance.root.mainloop()

        tk_thread = threading.Thread(target=run)
        tk_thread.daemon = True
        tk_thread.start()

        while not hasattr(cls, 'instance'):
            time.sleep(0.1)

        return cls.instance




# ## Start the Tkinter GUI and get a reference to the `GraphWindow` instance
# graph_window = GraphWindow.start()


# # # Simulating updates to the different graphs
# time.sleep(1)
# graph_window.update_graph1(10, 50)  
# graph_window.update_graph2(0.2, 0.5)
# graph_window.update_graph3(0.3, 1.2)
# time.sleep(1)
# graph_window.update_graph1(20, 40)  
# graph_window.update_graph2(0.3, 1.2)
# graph_window.update_graph3(-0.2, 2.5)
# time.sleep(1)
# graph_window.update_graph1(5, 60)  
# graph_window.update_graph2(0.7, 1.4)
# graph_window.update_graph3(0.1, 3.0) 

# print("Tkinter is running in a separate thread, and additional code can execute.")
# graph_window.save_graph()

# time.sleep(5)