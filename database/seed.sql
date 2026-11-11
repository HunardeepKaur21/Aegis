-- Simulated data: a river flood in a fictional district. All values are invented.
INSERT INTO disasters (id,name,type,severity,description,affected_people,started_at) VALUES
 (1,'Riverside Flood 2026','flood',4,'Heavy monsoon rain has pushed the river over its banks. Low-lying villages are cut off and two roads are underwater.',2340,datetime('now','-9 hours'));

INSERT INTO locations (id,disaster_id,name,type,latitude,longitude) VALUES
 (1,1,'Command Center','rescue_center',28.6139,77.2090),
 (2,1,'Central Hospital','hospital',28.6280,77.2200),
 (3,1,'Riverbank Village','village',28.5980,77.2300),
 (4,1,'Eastgate Junction','junction',28.6150,77.2350),
 (5,1,'North Bridge','bridge',28.6350,77.2050),
 (6,1,'Lakeside Colony','village',28.6050,77.1850),
 (7,1,'City School','shelter',28.6220,77.1900),
 (8,1,'Sports Complex','shelter',28.6000,77.2150),
 (9,1,'Supply Depot','depot',28.6250,77.2400),
 (10,1,'Hillview Settlement','village',28.6450,77.2250),
 (11,1,'Market Square','junction',28.6180,77.2150),
 (12,1,'Community Hall','shelter',28.6400,77.1950),
 (13,1,'Fire Station','rescue_center',28.6210,77.2270),
 (14,1,'Riverside Apartments','building',28.6090,77.2320);

INSERT INTO roads (from_id,to_id,distance_km,is_blocked) VALUES
 (1,11,1.2,0),(11,2,1.8,0),(11,4,2.3,0),(4,3,2.1,0),(4,9,2.6,0),
 (1,8,1.9,0),(8,3,2.8,1),(1,7,3.4,0),(7,6,2.5,0),(7,12,2.9,0),
 (12,5,2.2,0),(5,2,2.7,0),(2,10,2.4,0),(10,9,3.6,0),(5,1,3.0,0),
 (6,8,3.1,0),(11,8,2.0,0),(2,9,3.3,1),
 (13,11,1.4,0),(13,9,1.7,0),(14,3,1.4,0),(14,4,1.1,0);

INSERT INTO rescue_teams (id,name,team_type,type,members,capacity,status,availability,current_location_id,base_location_id) VALUES
 (1,'Alpha Medical Team','medical','medical',6,12,'available','available',2,2),
 (2,'Bravo Fire Rescue','fire','fire',7,10,'available','available',13,13),
 (3,'Charlie Flood Rescue','flood','boat',5,15,'deployed','deployed',1,1),
 (4,'Delta General Emergency','general','search',8,20,'available','available',1,1),
 (5,'Echo Fire & Rescue','fire','fire',6,8,'available','available',9,9),
 (6,'Foxtrot Medical','medical','medical',4,6,'offline','offline',2,2);

INSERT INTO rescue_requests (id,disaster_id,location_id,victim_name,requester_name,emergency_type,severity,affected_people,people_affected,description,required_assistance,medical_emergency,priority_score,urgency_score,status,assigned_team_id,created_at) VALUES
 (1,1,3,'Harpreet Kaur','Harpreet Kaur','Flood','Critical',42,42,'Families stranded on rooftops, water rising rapidly','Inflatable boat rescue and life jackets',1,85.0,85.0,'pending',NULL,datetime('now','-7 hours')),
 (2,1,6,'Rohan Mehta','Rohan Mehta','Medical','High',14,14,'Elderly residents trapped on first floor, diabetic emergencies','Oxygen and wheelchair extraction',1,71.0,71.0,'pending',NULL,datetime('now','-6 hours')),
 (3,1,10,'Sunita Devi','Sunita Devi','Building Collapse','High',60,60,'Landslide debris collapsed community porch, residents trapped','Heavy lifting gear and structural support',0,70.0,70.0,'pending',NULL,datetime('now','-5 hours')),
 (4,1,3,'Imran Sheikh','Imran Sheikh','Flood','High',18,18,'Boat needed for evacuation, young children present','Evacuation boat and blankets',0,65.0,65.0,'assigned',3,datetime('now','-4 hours')),
 (5,1,4,'Gurpreet Singh','Gurpreet Singh','Accident','Low',9,9,'Vehicles stuck in floodwater at junction, passengers safe on roof','Tow vehicle and traffic diversion',0,26.5,26.5,'pending',NULL,datetime('now','-3 hours')),
 (6,1,6,'Anjali Verma','Anjali Verma','Medical','Critical',1,1,'Pregnant woman in labor needs urgent hospital transfer','Advanced life support ambulance',1,74.5,74.5,'pending',NULL,datetime('now','-150 minutes')),
 (7,1,10,'Karan Malhotra','Karan Malhotra','Other','Medium',200,200,'No drinking water for 200 residents, dehydration spreading','Clean water tankers and oral rehydration salts',0,50.0,50.0,'pending',NULL,datetime('now','-2 hours')),
 (8,1,9,'Depot Supervisor','Depot Supervisor','Fire','Low',5,5,'Electrical short circuit in depot, smoke visible','Fire containment and extinguishers',0,25.5,25.5,'pending',NULL,datetime('now','-90 minutes')),
 (9,1,3,'Fatima Begum','Fatima Begum','Medical','High',11,11,'Medicine shortage, diabetic patients in distress','Cold storage insulin delivery',1,67.5,67.5,'pending',NULL,datetime('now','-1 hours')),
 (10,1,12,'Vikram Rao','Vikram Rao','Other','Low',85,85,'Power outage at shelter, backup generator needed','Fuel and 10kVA generator',0,40.0,40.0,'completed',4,datetime('now','-8 hours'));

INSERT INTO rescue_assignments (request_id,team_id,assigned_at,status,route_distance) VALUES
 (4,3,datetime('now','-4 hours'),'active',3.7),
 (10,4,datetime('now','-8 hours'),'completed',4.2);

INSERT INTO shelters (id,name,location_id,capacity,total_capacity,occupied,current_occupancy,has_medical,facilities,status) VALUES
 (1,'City School Shelter',7,400,400,310,310,0,'Classrooms, Community Kitchen, Water Purifier, Power Generator','Available'),
 (2,'Sports Complex Shelter',8,600,600,420,420,1,'Medical Bay, Intensive Care Cots, Emergency Power, Sanitation Units','Available'),
 (3,'Community Hall Shelter',12,150,150,148,148,0,'Emergency Cots, First Aid Supplies, Dry Food Storage','Limited Capacity'),
 (4,'Riverbank Relief Camp',3,200,200,200,200,0,'Temporary Tents, Basic Ration Depot, Clean Drinking Water','Full');

INSERT INTO resources (id,name,category,available_quantity,quantity,unit_weight,weight_kg,value_score,value,required_quantity,shelter_id) VALUES
 (1,'Rice sacks (25 kg)','Food',40,40,25.0,25.0,60,60,60,1),
 (2,'Ready-to-eat meal packs','Food',120,120,12.0,12.0,55,55,150,2),
 (3,'Baby food cartons','Food',30,30,8.0,8.0,70,70,35,1),
 (4,'Bottled water crates','Drinking Water',90,90,18.0,18.0,80,80,120,2),
 (5,'Water purification tablets','Drinking Water',200,200,2.0,2.0,65,65,180,3),
 (6,'Insulin cold-chain kit','Medicines',12,12,6.0,6.0,95,95,20,2),
 (7,'First-aid trauma kits','First Aid',60,60,4.0,4.0,75,75,50,1),
 (8,'Antibiotics boxes','Medicines',25,25,5.0,5.0,85,85,30,2),
 (9,'Life jackets (pack of 10)','Rescue Equipment',20,20,15.0,15.0,90,90,25,1),
 (10,'Inflatable rescue boats','Rescue Equipment',6,6,45.0,45.0,100,100,8,3),
 (11,'Portable generators','Rescue Equipment',8,8,38.0,38.0,70,70,10,3),
 (12,'Thermal fleece blankets','Blankets',100,100,10.0,10.0,50,50,150,1);

INSERT INTO resource_allocations (id,resource_id,shelter_id,request_id,allocated_quantity,quantity,total_weight,allocation_date,allocated_at) VALUES
 (1,1,1,NULL,10,10,250.0,datetime('now','-6 hours'),datetime('now','-6 hours')),
 (2,4,2,NULL,20,20,360.0,datetime('now','-4 hours'),datetime('now','-4 hours')),
 (3,7,2,NULL,15,15,60.0,datetime('now','-2 hours'),datetime('now','-2 hours'));

INSERT INTO settings (key,value) VALUES ('transport_capacity_kg','300');

INSERT INTO victims (id,name,age,gender,health_status,accompanying_people,emergency_category,current_location_id,medical_assistance,shelter_id,assigned_shelter_id,status) VALUES
 (1,'Manjit Kaur',67,'F','stable',2,'Medium',7,0,1,1,'sheltered'),
 (2,'Arjun Sharma',34,'M','stable',3,'Low',7,0,1,1,'sheltered'),
 (3,'Priya Nair',8,'F','stable',1,'High',7,0,1,1,'sheltered'),
 (4,'Deepak Yadav',45,'M','injured',0,'Critical',8,1,2,2,'sheltered'),
 (5,'Sana Khan',29,'F','stable',2,'Medium',8,0,2,2,'sheltered'),
 (6,'Gurdev Singh',72,'M','critical',1,'Critical',8,1,2,2,'sheltered'),
 (7,'Lakshmi Iyer',55,'F','injured',0,'High',8,1,2,2,'sheltered'),
 (8,'Rahul Gupta',12,'M','stable',4,'Low',12,0,3,3,'sheltered'),
 (9,'Meera Joshi',38,'F','stable',1,'Medium',12,0,3,3,'sheltered'),
 (10,'Baljit Singh',61,'M','injured',0,'High',7,1,1,1,'sheltered'),
 (11,'Neha Kapoor',26,'F','stable',2,'High',3,0,NULL,NULL,'awaiting_shelter'),
 (12,'Aman Chaudhary',5,'M','stable',3,'Medium',5,0,NULL,NULL,'awaiting_shelter'),
 (13,'Zoya Ansari',41,'F','injured',1,'Critical',6,1,NULL,NULL,'awaiting_shelter'),
 (14,'Harjot Gill',33,'M','stable',0,'Low',4,0,NULL,NULL,'awaiting_shelter'),
 (15,'Kavita Reddy',79,'F','critical',1,'Critical',9,1,NULL,NULL,'awaiting_shelter');

INSERT INTO simulation_events (disaster_id,category,message,created_at) VALUES
 (1,'system','Scenario "Riverside Flood 2026" activated',datetime('now','-9 hours')),
 (1,'road','Road Sports Complex to Riverbank Village blocked by floodwater',datetime('now','-6 hours')),
 (1,'rescue','Charlie Boat Unit deployed to Riverbank Village',datetime('now','-4 hours')),
 (1,'road','Road Central Hospital to Supply Depot blocked',datetime('now','-3 hours')),
 (1,'shelter','Community Hall Shelter reached 99% capacity',datetime('now','-2 hours')),
 (1,'resource','Insulin and cold-chain stock reported low',datetime('now','-1 hours'));
