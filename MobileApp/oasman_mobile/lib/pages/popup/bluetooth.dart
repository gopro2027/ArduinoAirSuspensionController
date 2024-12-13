import 'package:flutter/material.dart';

class BluetoothPopup extends StatelessWidget {
  final List<String>? devices;

  const BluetoothPopup({super.key, this.devices});

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(20),
      ),
      backgroundColor: Colors.black,
      child: Container(
        padding: const EdgeInsets.all(20),
        constraints: BoxConstraints(
          maxHeight: MediaQuery.of(context).size.height * 0.6,
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  "Connect to the controller",
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.close, color: Colors.white),
                  onPressed: () => Navigator.of(context).pop(),
                )
              ],
            ),
            const SizedBox(height: 20),
            devices == null || devices!.isEmpty
                ? Column(
                    children: [
                      const Text(
                        "Could not find any bluetooth devices. Check if bluetooth is enabled or try refreshing",
                        style: TextStyle(color: Colors.grey, fontSize: 14),
                        textAlign: TextAlign.center,
                      ),
                      const SizedBox(height: 20),
                      ElevatedButton(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.purple,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10), // Firkantet med runde hjørner
                          ),
                        ),
                        onPressed: () {
                          // Simulate refreshing devices
                          Navigator.of(context).pop();
                          showDialog(
                            context: context,
                            builder: (context) => BluetoothPopup(
                              devices: [
                                "TwojaStaraGra",
                                "JebanieNaMandolinie",
                                "Bakłażan",
                                "SurykatkaBT",
                                "Jesiotrowe granie",
                                "OkrągAdoracji",
                                "AirRide Controller",
                              ],
                            ),
                          );
                        },
                        child: const Text(
                          "Refresh",
                          style: TextStyle(color: Colors.black),
                        ),
                      ),
                    ],
                  )
                : Expanded(
                    child: ListView.separated(
                      itemCount: devices!.length,
                      separatorBuilder: (context, index) => const Divider(
                        color: Colors.grey,
                        height: 1,
                      ),
                      itemBuilder: (context, index) {
                        return ListTile(
                          title: Text(
                            devices![index],
                            style: const TextStyle(color: Colors.white),
                          ),
                          trailing: const Icon(
                            Icons.arrow_forward_ios,
                            color: Colors.purple,
                          ),
                          onTap: () {
                            // Logik for valg af enhed
                            Navigator.of(context).pop();
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(
                                content: Text('Connected to ${devices![index]}'),
                              ),
                            );
                          },
                        );
                      },
                    ),
                  ),
          ],
        ),
      ),
    );
  }
}
