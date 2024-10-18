import json
import matplotlib.pyplot as plt
import contextily as ctx
# Open the JSON file
filename="blueprint_test/1914_blueprint_nDevices_2000_nGateways_20.json"
with open(filename, 'r') as file:
    # Load the JSON data
    json_data = json.load(file)

# Extract gateway and device positions
gateway_positions = [(gateway['latitude'], gateway['longitude']) for gateway in json_data["network"]["gateways"]]
device_positions = [(device['latitude'], device['longitude']) for device in json_data["network"]["devices"]]

# Unzip the positions for plotting
gateway_lats, gateway_lons = zip(*gateway_positions)
device_lats, device_lons = zip(*device_positions)

# Create the plot
fig, ax = plt.subplots(figsize=(10, 8))

# Plot devices as red squares
ax.scatter(device_lons, device_lats, marker='s', color='red', label='Devices', zorder=2)

# Plot gateways as blue triangles
ax.scatter(gateway_lons, gateway_lats, marker='^', color='blue', label='Gateways', zorder=2)


# Add OpenStreetMap background
ctx.add_basemap(ax, crs="EPSG:4326", source=ctx.providers.OpenStreetMap.Mapnik)

# Add labels and title
ax.set_xlabel('Longitude')
ax.set_ylabel('Latitude')
ax.set_title('Device and Gateway Positions')
ax.legend()

# Show the plot
plt.show()
