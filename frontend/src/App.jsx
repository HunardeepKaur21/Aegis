import { Routes, Route, Navigate } from 'react-router-dom';
import Sidebar from './components/Sidebar.jsx';
import Dashboard from './pages/Dashboard.jsx';
import DisasterMap from './pages/DisasterMap.jsx';
import RescueRequests from './pages/RescueRequests.jsx';
import ResourceOptimizer from './pages/ResourceOptimizer.jsx';
import SheltersVictims from './pages/SheltersVictims.jsx';
import AlgorithmLab from './pages/AlgorithmLab.jsx';

export default function App() {
  return (
    <div className="shell">
      <Sidebar />
      <main className="main">
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route path="/map" element={<DisasterMap />} />
          <Route path="/requests" element={<RescueRequests />} />
          <Route path="/resources" element={<ResourceOptimizer />} />
          <Route path="/shelters" element={<SheltersVictims />} />
          <Route path="/lab" element={<AlgorithmLab />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </main>
    </div>
  );
}
