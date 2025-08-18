#!/usr/bin/env python3
"""
Test script for O3DE Vehicle Dynamics ROS2 AD System
Tests all functionality including manual override detection
AD System: Press 'A' key to engage autonomous driving mode
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32, Bool
from std_msgs.msg import Float32MultiArray
import time
import threading
import signal
import sys

class VehicleADTester(Node):
    def __init__(self):
        super().__init__('vehicle_ad_tester')
        
        # Publishers for vehicle control
        self.steering_pub = self.create_publisher(Float32, '/vehicle/steering_angle', 10)
        self.acceleration_pub = self.create_publisher(Float32, '/vehicle/acceleration', 10)
        self.ad_enable_pub = self.create_publisher(Bool, '/ad_enable', 10)
        
        # Subscribers for monitoring
        self.ad_status_sub = self.create_subscription(
            Bool, '/ad_status', self.ad_status_callback, 10)
        self.wheel_speeds_sub = self.create_subscription(
            Float32MultiArray, '/vehicle/wheel_speeds', self.wheel_speeds_callback, 10)
        
        # State tracking
        self.current_ad_status = False
        self.last_wheel_speeds = []
        self.ad_status_changes = []
        self.test_running = True
        
        self.get_logger().info("🚗 Vehicle AD Tester initialized 🤖")
    
    def ad_status_callback(self, msg):
        """Monitor AD status changes"""
        if msg.data != self.current_ad_status:
            timestamp = time.time()
            self.ad_status_changes.append((timestamp, msg.data))
            self.get_logger().info(f"🔄 AD Status changed: {self.current_ad_status} -> {msg.data}")
            self.current_ad_status = msg.data
    
    def wheel_speeds_callback(self, msg):
        """Monitor wheel speeds"""
        if len(msg.data) >= 4:
            self.last_wheel_speeds = list(msg.data)
    
    def wait_for_ad_status_change(self, timeout=10.0):
        """Wait for AD status to change, return True if it changed"""
        initial_status = self.current_ad_status
        start_time = time.time()
        
        self.get_logger().info(f"⏳ Waiting up to {timeout}s for AD status change from {initial_status}...")
        
        while (time.time() - start_time) < timeout:
            rclpy.spin_once(self, timeout_sec=0.1)
            if self.current_ad_status != initial_status:
                elapsed = time.time() - start_time
                self.get_logger().info(f"✅ AD status changed to {self.current_ad_status} after {elapsed:.2f}s")
                return True
        
        self.get_logger().warning(f"❌ No AD status change detected within {timeout}s")
        return False
    
    def send_steering_command(self, angle):
        """Send steering angle command"""
        msg = Float32()
        msg.data = angle
        self.steering_pub.publish(msg)
        self.get_logger().info(f"🎮 Sent steering: {angle:.2f} rad")
    
    def send_acceleration_command(self, accel):
        """Send acceleration command"""
        msg = Float32()
        msg.data = accel
        self.acceleration_pub.publish(msg)
        self.get_logger().info(f"⚡ Sent acceleration: {accel:.2f} m/s²")
    
    def set_ad_enable(self, enabled):
        """Enable or disable AD capability"""
        msg = Bool()
        msg.data = enabled
        self.ad_enable_pub.publish(msg)
        self.get_logger().info(f"🔧 Set AD enable: {enabled}")
    
    def test_manual_override_detection(self):
        """Test manual override detection with timeout monitoring - Press A key to engage AD"""
        self.get_logger().info("=== 🔍 Manual Override Detection Test ===")
        
        # Setup AD
        self.set_ad_enable(True)
        time.sleep(1.0)
        
        self.get_logger().info("📋 Step 1: Press A key in O3DE to engage AD...")
        if not self.wait_for_ad_status_change(15.0):
            self.get_logger().error("❌ AD engagement failed")
            return
        
        self.get_logger().info("📋 Step 2: Monitoring for manual override...")
        self.get_logger().info("🎮 Press ANY ARROW KEY in O3DE within 10 seconds...")
        
        # Monitor for exactly 10 seconds
        if self.wait_for_ad_status_change(10.0):
            self.get_logger().info("✅ Manual override detected! AD disengaged correctly.")
            
            # Test re-engagement after timeout
            self.get_logger().info("📋 Step 3: Testing re-engagement after timeout...")
            self.get_logger().info("⏱️  Waiting 3 seconds for manual input timeout...")
            time.sleep(3.0)
            
            self.get_logger().info("📋 Step 4: Press A key to re-engage AD...")
            if self.wait_for_ad_status_change(10.0):
                self.get_logger().info("✅ AD re-engagement successful!")
                return True
            else:
                self.get_logger().warning("❌ AD re-engagement failed")
                return False
        else:
            self.get_logger().warning("❌ No manual override detected within 10s")
            return False
    
    def test_ros2_control_sequence(self):
        """Test ROS2 vehicle control sequence - Press A key to engage AD if needed"""
        self.get_logger().info("=== 🎮 ROS2 Control Sequence Test ===")
        
        # Ensure AD is enabled and engaged
        self.set_ad_enable(True)
        time.sleep(1.0)
        
        if not self.current_ad_status:
            self.get_logger().info("📋 Press A key to engage AD for this test...")
            if not self.wait_for_ad_status_change(15.0):
                self.get_logger().error("❌ AD not engaged - test aborted")
                return
        
        self.get_logger().info("🚗 Starting vehicle control sequence...")
        
        # Steering test sequence
        steering_sequence = [0.0, 0.3, 0.0, -0.3, 0.0]
        for i, angle in enumerate(steering_sequence):
            self.get_logger().info(f"📋 Steering step {i+1}/5: {angle:.1f} rad")
            self.send_steering_command(angle)
            time.sleep(2.0)
        
        # Acceleration test sequence  
        accel_sequence = [0.0, 1.0, 2.0, 0.0, -1.0, 0.0]
        for i, accel in enumerate(accel_sequence):
            self.get_logger().info(f"📋 Acceleration step {i+1}/6: {accel:.1f} m/s²")
            self.send_acceleration_command(accel)
            time.sleep(4.0)  # Increased from 2.0 to 4.0 seconds for better vehicle movement
            
            # Show wheel speeds during acceleration
            if self.last_wheel_speeds:
                speeds = [f"{s:.2f}" for s in self.last_wheel_speeds]
                self.get_logger().info(f"📊 Wheel speeds: [{', '.join(speeds)}] km/h")
        
        self.get_logger().info("✅ ROS2 control sequence complete!")
    
    def show_status(self):
        """Show current system status"""
        self.get_logger().info("=== 📊 Current System Status ===")
        self.get_logger().info(f"🤖 AD Status: {self.current_ad_status}")
        
        if self.last_wheel_speeds:
            speeds_str = ", ".join([f"{speed:.3f}" for speed in self.last_wheel_speeds])
            self.get_logger().info(f"🏁 Wheel Speeds (km/h): [{speeds_str}] FL,FR,RL,RR")
        else:
            self.get_logger().info("📊 No wheel speed data available")
        
        self.get_logger().info(f"�� Total AD status changes: {len(self.ad_status_changes)}")

def main():
    # Initialize ROS2
    rclpy.init()
    
    try:
        # Create test node
        tester = VehicleADTester()
        
        print("🚗 Vehicle AD System Tester Started 🤖")
        print("Make sure O3DE is running in game mode with the vehicle loaded!")
        print("AD Engagement: Press 'A' key in O3DE to engage autonomous driving")
        print("\n" + "="*60)
        print("Test Menu:")
        print("1 - Manual Override Detection Test (recommended first test)")
        print("2 - ROS2 Control Sequence Test")  
        print("3 - Show Current Status")
        print("4 - Emergency Stop")
        print("q - Quit")
        print("="*60)
        
        # Start ROS2 spinning in background
        spin_thread = threading.Thread(target=lambda: rclpy.spin(tester), daemon=True)
        spin_thread.start()
        
        # Wait for initial connections
        time.sleep(2.0)
        
        # Interactive test loop
        while tester.test_running:
            try:
                choice = input("\nEnter test choice: ").strip().lower()
                
                if choice == '1':
                    tester.test_manual_override_detection()
                elif choice == '2':
                    tester.test_ros2_control_sequence()
                elif choice == '3':
                    tester.show_status()
                elif choice == '4':
                    tester.set_ad_enable(False)
                    tester.send_acceleration_command(0.0)
                    tester.send_steering_command(0.0)
                    tester.get_logger().warning("🛑 Emergency stop executed")
                elif choice == 'q':
                    tester.get_logger().info("👋 Exiting test script...")
                    break
                else:
                    print("❌ Invalid choice, please try again")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                tester.get_logger().error(f"❌ Test error: {e}")
        
    except KeyboardInterrupt:
        print("\n👋 Test script interrupted")
    finally:
        if 'tester' in locals():
            tester.destroy_node()
        rclpy.shutdown()
        print("🏁 Test script shutdown complete")

if __name__ == '__main__':
    main()
