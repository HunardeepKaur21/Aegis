# AEGIS Setup & Execution Guide (Frontend + Backend + Database)

Aapka AEGIS project poori tarah se configured aur tested hai.

---

## ⚡ Sabse Asaan Tareeka (1-Click Run)

Aapko bas **`START_ALL.bat`** par double click karna hai (chahe root folder `aegis-step2` me ho ya `aegis` folder me).
Ye script automatically:
1. C++ Crow Backend (`http://localhost:8080`) ko alag window me start karega.
2. React + Vite Frontend (`http://localhost:5173`) ko alag window me start karega.
3. Aapke browser me automatic **http://localhost:5173** open kar dega!

---

## 🛠️ Manually Run Karne Ka Tareeka (Step-by-Step)

Agar aap manually dono ko alag-alag terminals se chalana chahein:

### Step 1: Backend Start Karein (Port 8080)
1. Terminal open karein aur backend directory me jayein:
   ```cmd
   cd backend
   ```
2. Server run karein:
   ```cmd
   build\bin\aegis_server.exe ..\database\aegis.db ..\database 8080
   ```
   *(Ya fir simple **`start_backend.bat`** par double click karein)*

### Step 2: Frontend Start Karein (Port 5173)
1. Ek naya terminal open karein aur frontend directory me jayein:
   ```cmd
   cd frontend
   ```
2. Frontend dev server start karein:
   ```cmd
   npm run dev
   ```
   *(Ya fir simple **`start_frontend.bat`** par double click karein)*

3. Browser me open karein:
   👉 **http://localhost:5173**

---

## 🗄️ Database (SQLite) Information
- **Database file:** `aegis/database/aegis.db`
- **Schema & Seed:** `aegis/database/schema.sql` aur `aegis/database/seed.sql`
- Backend start hone par automatically check karta hai aur agar database empty ya fresh ho to seed scenario load kar deta hai.
- **Database Reset:** Agar aapko kabhi fresh state par reset karna ho, to backend band karke `aegis\database\aegis.db` file delete kar dein aur backend dubara start karein.

---

## 🧪 Testing
Sare C++ DSA Unit tests (`test_graph`, `test_rescue`, `test_resource`, `test_shelter`, `test_lab`) verify kiye ja chuke hain aur pass ho rahe hain.
Aap unhe manually bhi run kar sakte hain:
```cmd
cd "C:\Users\Hunardeep Kaur\Downloads\aegis-step2\aegis\backend\build\bin"
.\test_graph.exe
.\test_rescue.exe
.\test_resource.exe
.\test_shelter.exe
.\test_lab.exe
```
