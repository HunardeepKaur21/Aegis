import { useCallback, useEffect, useState } from 'react';
import {
  assignShelter,
  createShelter,
  createVictim,
  deleteShelter,
  deleteVictim,
  getShelterOverview,
  getShelters,
  getVictims,
  recommendShelter,
  updateShelter,
  updateVictim,
} from '../services/shelterService.js';
import { getLocations } from '../services/mapService.js';

const CATEGORIES = ['All', 'Critical', 'High', 'Medium', 'Low'];
const STATUSES = [
  { id: 'All', label: 'All Statuses' },
  { id: 'awaiting_shelter', label: 'Awaiting Shelter' },
  { id: 'sheltered', label: 'Sheltered' },
];

export default function SheltersVictims() {
  // Active Tab: 'victims' | 'shelters'
  const [activeTab, setActiveTab] = useState('victims');

  // Core Data
  const [victims, setVictims] = useState([]);
  const [shelters, setShelters] = useState([]);
  const [locations, setLocations] = useState([]);
  const [overview, setOverview] = useState({
    total_victims: 0,
    awaiting_shelter: 0,
    sheltered_victims: 0,
    total_shelters: 0,
    total_capacity: 0,
    current_occupancy: 0,
    available_capacity: 0,
    full_shelters: 0,
    available_shelters: 0,
  });

  // Filters & Search
  const [searchVictim, setSearchVictim] = useState('');
  const [categoryFilter, setCategoryFilter] = useState('All');
  const [statusFilter, setStatusFilter] = useState('All');

  const [searchShelter, setSearchShelter] = useState('');
  const [shelterStatusFilter, setShelterStatusFilter] = useState('All');

  // Modals
  const [showAddVictimModal, setShowAddVictimModal] = useState(false);
  const [showEditVictimModal, setShowEditVictimModal] = useState(false);
  const [showAddShelterModal, setShowAddShelterModal] = useState(false);
  const [showEditShelterModal, setShowEditShelterModal] = useState(false);
  const [showRecommendModal, setShowRecommendModal] = useState(false);

  // Active Selected Items
  const [selectedVictim, setSelectedVictim] = useState(null);
  const [selectedShelter, setSelectedShelter] = useState(null);
  const [recommendationData, setRecommendationData] = useState(null);
  const [loadingRecommend, setLoadingRecommend] = useState(false);

  // Forms
  const [victimForm, setVictimForm] = useState({
    name: '',
    age: 30,
    gender: 'O',
    health_status: 'stable',
    accompanying_people: 0,
    emergency_category: 'Medium',
    current_location_id: 1,
    medical_assistance: false,
    assigned_shelter_id: '',
  });

  const [shelterForm, setShelterForm] = useState({
    name: '',
    location_id: 1,
    total_capacity: 200,
    has_medical: false,
    facilities: 'Clean Water, Cots, Power Backup, Sanitation',
  });

  // UI Toast & Loading
  const [toast, setToast] = useState(null);
  const [loading, setLoading] = useState(false);

  const showNotification = (text, type = 'success') => {
    setToast({ text, type });
    setTimeout(() => setToast(null), 3000);
  };

  const loadData = useCallback(async () => {
    setLoading(true);
    try {
      const [vList, sList, ovData, locList] = await Promise.all([
        getVictims({
          category: categoryFilter,
          status: statusFilter,
          search: searchVictim.trim(),
        }),
        getShelters(),
        getShelterOverview(),
        getLocations(),
      ]);
      setVictims(vList || []);
      setShelters(sList || []);
      setOverview(ovData || {});
      setLocations(locList || []);
    } catch (err) {
      showNotification('Error loading data: ' + err.message, 'error');
    } finally {
      setLoading(false);
    }
  }, [categoryFilter, statusFilter, searchVictim]);

  useEffect(() => {
    loadData();
  }, [loadData]);

  // Handle Register Victim
  const handleCreateVictim = async (e) => {
    e.preventDefault();
    if (!victimForm.name.trim()) {
      showNotification('Please enter victim name or identifier.', 'error');
      return;
    }
    try {
      await createVictim({
        name: victimForm.name.trim(),
        age: Number(victimForm.age),
        gender: victimForm.gender,
        health_status: victimForm.health_status,
        accompanying_people: Number(victimForm.accompanying_people),
        emergency_category: victimForm.emergency_category,
        current_location_id: Number(victimForm.current_location_id),
        medical_assistance: Boolean(victimForm.medical_assistance),
        assigned_shelter_id: victimForm.assigned_shelter_id ? Number(victimForm.assigned_shelter_id) : undefined,
      });
      showNotification(`Registered victim "${victimForm.name}" successfully.`);
      setShowAddVictimModal(false);
      setVictimForm({
        name: '',
        age: 30,
        gender: 'O',
        health_status: 'stable',
        accompanying_people: 0,
        emergency_category: 'Medium',
        current_location_id: locations[0]?.id || 1,
        medical_assistance: false,
        assigned_shelter_id: '',
      });
      loadData();
    } catch (err) {
      showNotification('Failed to register victim: ' + err.message, 'error');
    }
  };

  // Handle Edit Victim
  const handleUpdateVictim = async (e) => {
    e.preventDefault();
    if (!selectedVictim) return;
    try {
      await updateVictim(selectedVictim.id, {
        name: victimForm.name.trim(),
        age: Number(victimForm.age),
        gender: victimForm.gender,
        health_status: victimForm.health_status,
        accompanying_people: Number(victimForm.accompanying_people),
        emergency_category: victimForm.emergency_category,
        current_location_id: Number(victimForm.current_location_id),
        medical_assistance: Boolean(victimForm.medical_assistance),
        assigned_shelter_id: victimForm.assigned_shelter_id ? Number(victimForm.assigned_shelter_id) : undefined,
      });
      showNotification(`Updated victim #${selectedVictim.id}.`);
      setShowEditVictimModal(false);
      loadData();
    } catch (err) {
      showNotification('Failed to update victim: ' + err.message, 'error');
    }
  };

  // Handle Delete Victim
  const handleDeleteVictim = async (victim) => {
    if (!window.confirm(`Are you sure you want to delete victim record "${victim.name}"?`)) return;
    try {
      await deleteVictim(victim.id);
      showNotification(`Victim #${victim.id} removed from system.`);
      loadData();
    } catch (err) {
      showNotification('Failed to delete victim: ' + err.message, 'error');
    }
  };

  // Handle Add Shelter
  const handleCreateShelter = async (e) => {
    e.preventDefault();
    if (!shelterForm.name.trim()) {
      showNotification('Please enter a shelter name.', 'error');
      return;
    }
    try {
      await createShelter({
        name: shelterForm.name.trim(),
        location_id: Number(shelterForm.location_id),
        total_capacity: Number(shelterForm.total_capacity),
        has_medical: Boolean(shelterForm.has_medical),
        facilities: shelterForm.facilities.trim(),
      });
      showNotification(`Shelter "${shelterForm.name}" created successfully.`);
      setShowAddShelterModal(false);
      setShelterForm({
        name: '',
        location_id: locations[0]?.id || 1,
        total_capacity: 200,
        has_medical: false,
        facilities: 'Clean Water, Cots, Power Backup, Sanitation',
      });
      loadData();
    } catch (err) {
      showNotification('Failed to create shelter: ' + err.message, 'error');
    }
  };

  // Handle Edit Shelter
  const handleUpdateShelter = async (e) => {
    e.preventDefault();
    if (!selectedShelter) return;
    try {
      await updateShelter(selectedShelter.id, {
        name: shelterForm.name.trim(),
        location_id: Number(shelterForm.location_id),
        total_capacity: Number(shelterForm.total_capacity),
        has_medical: Boolean(shelterForm.has_medical),
        facilities: shelterForm.facilities.trim(),
      });
      showNotification(`Updated shelter #${selectedShelter.id}.`);
      setShowEditShelterModal(false);
      loadData();
    } catch (err) {
      showNotification('Failed to update shelter: ' + err.message, 'error');
    }
  };

  // Handle Delete Shelter
  const handleDeleteShelter = async (shelter) => {
    if (shelter.current_occupancy > 0) {
      showNotification(`Cannot delete shelter with ${shelter.current_occupancy} active occupants.`, 'error');
      return;
    }
    if (!window.confirm(`Are you sure you want to decommission shelter "${shelter.name}"?`)) return;
    try {
      await deleteShelter(shelter.id);
      showNotification(`Shelter "${shelter.name}" decommissioned.`);
      loadData();
    } catch (err) {
      showNotification('Failed to delete shelter: ' + err.message, 'error');
    }
  };

  // Open Smart Recommendation Modal
  const handleOpenRecommend = async (victim) => {
    setSelectedVictim(victim);
    setShowRecommendModal(true);
    setLoadingRecommend(true);
    setRecommendationData(null);
    try {
      const rec = await recommendShelter(victim.id);
      setRecommendationData(rec);
    } catch (err) {
      showNotification('Recommendation error: ' + err.message, 'error');
    } finally {
      setLoadingRecommend(false);
    }
  };

  // Assign Victim to Shelter
  const handleAssignVictim = async (victimId, shelterId) => {
    try {
      const res = await assignShelter(victimId, shelterId);
      showNotification(res.message || 'Assigned to shelter successfully.');
      setShowRecommendModal(false);
      loadData();
    } catch (err) {
      showNotification('Failed to assign shelter: ' + err.message, 'error');
    }
  };

  const openEditVictim = (v) => {
    setSelectedVictim(v);
    setVictimForm({
      name: v.name,
      age: v.age,
      gender: v.gender || 'O',
      health_status: v.health_status || 'stable',
      accompanying_people: v.accompanying_people || 0,
      emergency_category: v.emergency_category || 'Medium',
      current_location_id: v.current_location_id || 1,
      medical_assistance: Boolean(v.medical_assistance),
      assigned_shelter_id: v.assigned_shelter_id ? String(v.assigned_shelter_id) : '',
    });
    setShowEditVictimModal(true);
  };

  const openEditShelter = (s) => {
    setSelectedShelter(s);
    setShelterForm({
      name: s.name,
      location_id: s.location_id,
      total_capacity: s.total_capacity,
      has_medical: s.has_medical,
      facilities: s.facilities || '',
    });
    setShowEditShelterModal(true);
  };

  // Filtered Shelters list
  const filteredShelters = shelters.filter((s) => {
    const matchSearch =
      !searchShelter.trim() ||
      s.name.toLowerCase().includes(searchShelter.toLowerCase()) ||
      (s.location_name && s.location_name.toLowerCase().includes(searchShelter.toLowerCase()));

    let matchStatus = true;
    if (shelterStatusFilter === 'Available') matchStatus = s.status === 'Available';
    else if (shelterStatusFilter === 'Limited') matchStatus = s.status === 'Limited Capacity';
    else if (shelterStatusFilter === 'Full') matchStatus = s.status === 'Full';

    return matchSearch && matchStatus;
  });

  const getCategoryClass = (cat = '') => {
    const c = cat.toLowerCase();
    if (c === 'critical') return 'critical';
    if (c === 'high') return 'warning';
    if (c === 'medium') return 'info';
    return 'muted';
  };

  return (
    <div className="page-shell">
      {/* Toast Notification Banner */}
      {toast && (
        <div className={`toast-banner ${toast.type}`}>
          <span>{toast.text}</span>
          <button className="toast-close" onClick={() => setToast(null)}>
            ×
          </button>
        </div>
      )}

      {/* Page Header */}
      <div className="section-head">
        <div>
          <h1 className="page-title">Shelters & Victims</h1>
          <p className="page-subtitle">
            Register affected people, track shelter capacity and find the best shelter for each person.
          </p>
        </div>
        <div className="actions" style={{ display: 'flex', gap: '10px' }}>
          {activeTab === 'victims' ? (
            <button className="btn btn-primary" onClick={() => setShowAddVictimModal(true)}>
              + Register Victim
            </button>
          ) : (
            <button className="btn btn-primary" onClick={() => setShowAddShelterModal(true)}>
              + Add Emergency Shelter
            </button>
          )}
        </div>
      </div>

      {/* KPI Overview Summary Cards */}
      <div className="kpi-grid">
        <div className="kpi-card">
          <span className="label">Registered Victims</span>
          <span className="val">{overview.total_victims || 0}</span>
          <span className="sub">Individuals</span>
        </div>
        <div className="kpi-card">
          <span className="label">Awaiting Shelter</span>
          <span className="val" style={{ color: overview.awaiting_shelter > 0 ? 'var(--warning)' : 'var(--text)' }}>
            {overview.awaiting_shelter || 0}
          </span>
          <span className="sub">Needs Placement</span>
        </div>
        <div className="kpi-card">
          <span className="label">Total Shelters</span>
          <span className="val">{overview.total_shelters || 0}</span>
          <span className="sub">Relief Centers</span>
        </div>
        <div className="kpi-card">
          <span className="label">Available Capacity</span>
          <span className="val" style={{ color: 'var(--success)' }}>
            {overview.available_capacity?.toLocaleString() || 0}
          </span>
          <span className="sub">of {overview.total_capacity?.toLocaleString() || 0} beds</span>
        </div>
        <div className="kpi-card">
          <span className="label">Full Shelters</span>
          <span className="val" style={{ color: overview.full_shelters > 0 ? 'var(--critical)' : 'var(--text)' }}>
            {overview.full_shelters || 0}
          </span>
          <span className="sub">100% Capacity</span>
        </div>
      </div>

      {/* Main Tab Navigation */}
      <div style={{ display: 'flex', gap: '12px', borderBottom: '1px solid var(--line)', marginBottom: '20px' }}>
        <button
          className={`btn ${activeTab === 'victims' ? 'btn-primary' : 'btn-secondary'}`}
          style={{ borderRadius: '8px 8px 0 0', borderBottom: 0, padding: '10px 20px', fontSize: '13.5px' }}
          onClick={() => setActiveTab('victims')}
        >
          👥 Victim Management ({victims.length})
        </button>
        <button
          className={`btn ${activeTab === 'shelters' ? 'btn-primary' : 'btn-secondary'}`}
          style={{ borderRadius: '8px 8px 0 0', borderBottom: 0, padding: '10px 20px', fontSize: '13.5px' }}
          onClick={() => setActiveTab('shelters')}
        >
          🏠 Shelter Management ({shelters.length})
        </button>
      </div>

      {/* =========================================================================
          TAB 1: VICTIM MANAGEMENT
          ========================================================================= */}
      {activeTab === 'victims' && (
        <div className="card">
          <div className="card-head">
            <div className="title">
              <span>📋 Registered Evacuees & Victims</span>
            </div>
            <span className="tag">AVL & BST Indexed</span>
          </div>

          {/* Filters & Search Toolbar */}
          <div
            style={{
              display: 'flex',
              flexWrap: 'wrap',
              gap: '12px',
              alignItems: 'center',
              justifyContent: 'space-between',
              marginBottom: '16px',
            }}
          >
            {/* Search Box */}
            <div style={{ flex: '1 1 240px', maxWidth: '300px' }}>
              <input
                type="text"
                placeholder="🔍 Search victim by ID or name..."
                value={searchVictim}
                onChange={(e) => setSearchVictim(e.target.value)}
                style={{
                  width: '100%',
                  background: 'var(--bg)',
                  border: '1px solid var(--line)',
                  color: 'var(--text)',
                  padding: '8px 12px',
                  borderRadius: '6px',
                  fontSize: '13px',
                }}
              />
            </div>

            {/* Category Pills */}
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
              {CATEGORIES.map((cat) => (
                <button
                  key={cat}
                  className={`btn ${categoryFilter === cat ? 'btn-primary' : 'btn-secondary'}`}
                  style={{ fontSize: '11.5px', padding: '5px 10px' }}
                  onClick={() => setCategoryFilter(cat)}
                >
                  {cat}
                </button>
              ))}
            </div>

            {/* Status Selector */}
            <div style={{ display: 'flex', gap: '6px' }}>
              {STATUSES.map((st) => (
                <button
                  key={st.id}
                  className={`btn ${statusFilter === st.id ? 'btn-primary' : 'btn-secondary'}`}
                  style={{ fontSize: '11.5px', padding: '5px 10px' }}
                  onClick={() => setStatusFilter(st.id)}
                >
                  {st.label}
                </button>
              ))}
            </div>
          </div>

          {/* Victims Data Table */}
          <div style={{ overflowX: 'auto' }}>
            <table className="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>Name</th>
                  <th>Age/Gender</th>
                  <th>Family Headcount</th>
                  <th>Category</th>
                  <th>Current Location</th>
                  <th>Medical Assistance</th>
                  <th>Assigned Shelter</th>
                  <th>Status</th>
                  <th style={{ textAlign: 'right' }}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {victims.length === 0 ? (
                  <tr>
                    <td colSpan="10" style={{ textAlign: 'center', padding: '32px', color: 'var(--muted)' }}>
                      No victims found matching criteria.
                    </td>
                  </tr>
                ) : (
                  victims.map((v) => (
                    <tr key={v.id}>
                      <td style={{ color: 'var(--muted)', fontSize: '12px' }}>#{v.id}</td>
                      <td>
                        <strong style={{ color: 'var(--text)' }}>{v.name}</strong>
                      </td>
                      <td style={{ fontSize: '12.5px' }}>
                        {v.age} yrs • {v.gender}
                      </td>
                      <td>
                        <span style={{ fontWeight: 600 }}>{v.total_people}</span>{' '}
                        <small style={{ color: 'var(--muted)' }}>
                          ({v.accompanying_people} family)
                        </small>
                      </td>
                      <td>
                        <span className={`status-pill ${getCategoryClass(v.emergency_category)}`}>
                          {v.emergency_category}
                        </span>
                      </td>
                      <td style={{ fontSize: '12.5px', color: 'var(--muted)' }}>
                        {v.location_name}
                      </td>
                      <td>
                        {v.medical_assistance ? (
                          <span style={{ color: 'var(--critical)', fontWeight: 600, fontSize: '12px' }}>
                            🚑 Required
                          </span>
                        ) : (
                          <span style={{ color: 'var(--muted)', fontSize: '12px' }}>Standard</span>
                        )}
                      </td>
                      <td>
                        {v.assigned_shelter_id ? (
                          <span style={{ color: 'var(--primary)', fontWeight: 600, fontSize: '12.5px' }}>
                            {v.assigned_shelter_name}
                          </span>
                        ) : (
                          <span style={{ color: 'var(--warning)', fontSize: '12px', fontStyle: 'italic' }}>
                            ⚠️ Awaiting Shelter
                          </span>
                        )}
                      </td>
                      <td>
                        <span
                          className={`status-pill ${
                            v.status === 'sheltered' ? 'success' : 'warning'
                          }`}
                        >
                          {v.status === 'sheltered' ? 'Sheltered' : 'Awaiting Shelter'}
                        </span>
                      </td>
                      <td style={{ textAlign: 'right', whiteSpace: 'nowrap' }}>
                        <button
                          className="btn btn-secondary"
                          style={{ padding: '4px 8px', fontSize: '11.5px', marginRight: '6px' }}
                          onClick={() => handleOpenRecommend(v)}
                          title="Smart Dijkstra Shelter Recommendation"
                        >
                          🧭 Find Shelter
                        </button>
                        <button
                          className="btn btn-secondary"
                          style={{ padding: '4px 8px', fontSize: '11.5px', marginRight: '6px' }}
                          onClick={() => openEditVictim(v)}
                          title="Edit victim"
                        >
                          ✏️
                        </button>
                        <button
                          className="btn btn-secondary"
                          style={{ padding: '4px 8px', fontSize: '11.5px', color: 'var(--critical)' }}
                          onClick={() => handleDeleteVictim(v)}
                          title="Delete victim"
                        >
                          🗑️
                        </button>
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* =========================================================================
          TAB 2: SHELTER MANAGEMENT
          ========================================================================= */}
      {activeTab === 'shelters' && (
        <div className="card">
          <div className="card-head">
            <div className="title">
              <span>🏠 Emergency Relief Shelters</span>
            </div>
            <span className="tag">{filteredShelters.length} Active Centers</span>
          </div>

          {/* Filters & Search Toolbar */}
          <div
            style={{
              display: 'flex',
              flexWrap: 'wrap',
              gap: '12px',
              alignItems: 'center',
              justifyContent: 'space-between',
              marginBottom: '16px',
            }}
          >
            {/* Search Box */}
            <div style={{ flex: '1 1 240px', maxWidth: '300px' }}>
              <input
                type="text"
                placeholder="🔍 Search shelter by name or location..."
                value={searchShelter}
                onChange={(e) => setSearchShelter(e.target.value)}
                style={{
                  width: '100%',
                  background: 'var(--bg)',
                  border: '1px solid var(--line)',
                  color: 'var(--text)',
                  padding: '8px 12px',
                  borderRadius: '6px',
                  fontSize: '13px',
                }}
              />
            </div>

            {/* Status Pills */}
            <div style={{ display: 'flex', gap: '6px' }}>
              {['All', 'Available', 'Limited', 'Full'].map((st) => (
                <button
                  key={st}
                  className={`btn ${shelterStatusFilter === st ? 'btn-primary' : 'btn-secondary'}`}
                  style={{ fontSize: '11.5px', padding: '5px 10px' }}
                  onClick={() => setShelterStatusFilter(st)}
                >
                  {st}
                </button>
              ))}
            </div>
          </div>

          {/* Shelters Grid Cards */}
          <div
            style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fill, minmax(320px, 1fr))',
              gap: '16px',
            }}
          >
            {filteredShelters.map((s) => {
              const occPct = s.occupancy_pct.toFixed(1);
              const isFull = s.status === 'Full' || s.current_occupancy >= s.total_capacity;
              const isLimited = !isFull && s.status === 'Limited Capacity';

              return (
                <div
                  key={s.id}
                  style={{
                    background: 'var(--bg)',
                    border: '1px solid var(--line)',
                    borderRadius: '8px',
                    padding: '16px',
                    display: 'flex',
                    flexDirection: 'column',
                    justifyContent: 'space-between',
                  }}
                >
                  <div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                      <div>
                        <h3 style={{ fontSize: '15px', color: 'var(--text)', margin: '0 0 4px 0' }}>
                          {s.name}
                        </h3>
                        <div style={{ fontSize: '12px', color: 'var(--muted)' }}>
                          📍 {s.location_name}
                        </div>
                      </div>
                      <span
                        className={`status-pill ${
                          isFull ? 'critical' : isLimited ? 'warning' : 'success'
                        }`}
                      >
                        {s.status}
                      </span>
                    </div>

                    {/* Capacity Progress Bar */}
                    <div style={{ margin: '14px 0 10px 0' }}>
                      <div
                        style={{
                          display: 'flex',
                          justifyContent: 'space-between',
                          fontSize: '12px',
                          marginBottom: '4px',
                        }}
                      >
                        <span style={{ color: 'var(--muted)' }}>Occupancy</span>
                        <strong style={{ color: isFull ? 'var(--critical)' : 'var(--text)' }}>
                          {s.current_occupancy} / {s.total_capacity} beds ({occPct}%)
                        </strong>
                      </div>
                      <div
                        style={{
                          height: '8px',
                          background: 'rgba(255, 255, 255, 0.08)',
                          borderRadius: '4px',
                          overflow: 'hidden',
                        }}
                      >
                        <div
                          style={{
                            height: '100%',
                            width: `${Math.min(100, s.occupancy_pct)}%`,
                            borderRadius: '4px',
                            background: isFull
                              ? 'var(--critical)'
                              : isLimited
                              ? 'var(--warning)'
                              : 'var(--primary)',
                          }}
                        />
                      </div>
                      <div
                        style={{
                          display: 'flex',
                          justifyContent: 'space-between',
                          fontSize: '11px',
                          color: 'var(--muted)',
                          marginTop: '4px',
                        }}
                      >
                        <span>Available: {s.available_capacity} spots</span>
                        {s.has_medical && <span style={{ color: '#C084FC' }}>🏥 Medical Bay</span>}
                      </div>
                    </div>

                    {/* Facilities Tags */}
                    {s.facilities && (
                      <div style={{ fontSize: '11.5px', color: 'var(--muted)', margin: '8px 0' }}>
                        <strong>Facilities:</strong> {s.facilities}
                      </div>
                    )}
                  </div>

                  {/* Actions */}
                  <div
                    style={{
                      display: 'flex',
                      justifyContent: 'flex-end',
                      gap: '8px',
                      borderTop: '1px solid rgba(255, 255, 255, 0.06)',
                      paddingTop: '12px',
                      marginTop: '12px',
                    }}
                  >
                    <button
                      className="btn btn-secondary"
                      style={{ padding: '4px 10px', fontSize: '12px' }}
                      onClick={() => openEditShelter(s)}
                    >
                      ✏️ Edit Shelter
                    </button>
                    <button
                      className="btn btn-secondary"
                      style={{ padding: '4px 10px', fontSize: '12px', color: 'var(--critical)' }}
                      onClick={() => handleDeleteShelter(s)}
                      disabled={s.current_occupancy > 0}
                      title={s.current_occupancy > 0 ? 'Cannot delete shelter with active occupants' : 'Delete shelter'}
                    >
                      🗑️ Decommission
                    </button>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 1: SMART SHELTER RECOMMENDATION (DIJKSTRA)
          ========================================================================= */}
      {showRecommendModal && selectedVictim && (
        <div className="modal-backdrop" onClick={() => setShowRecommendModal(false)}>
          <div className="modal-window" style={{ maxWidth: '640px' }} onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>🧭 Smart Shelter Recommendation</h3>
              <button className="modal-close" onClick={() => setShowRecommendModal(false)}>
                ×
              </button>
            </div>
            <div className="modal-body">
              {/* Victim Context */}
              <div
                style={{
                  background: 'var(--bg)',
                  padding: '12px',
                  borderRadius: '6px',
                  border: '1px solid var(--line)',
                  marginBottom: '16px',
                }}
              >
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <div>
                    <strong style={{ fontSize: '14.5px', color: 'var(--text)' }}>
                      {selectedVictim.name}
                    </strong>{' '}
                    <span style={{ fontSize: '12px', color: 'var(--muted)' }}>
                      (Victim #{selectedVictim.id})
                    </span>
                    <div style={{ fontSize: '12px', color: 'var(--muted)', marginTop: '2px' }}>
                      📍 Location: <strong>{selectedVictim.location_name}</strong> • Headcount needed:{' '}
                      <strong style={{ color: 'var(--text)' }}>
                        {selectedVictim.total_people} spots
                      </strong>
                    </div>
                  </div>
                  <span className={`status-pill ${getCategoryClass(selectedVictim.emergency_category)}`}>
                    {selectedVictim.emergency_category}
                  </span>
                </div>
              </div>

              {loadingRecommend ? (
                <div style={{ textAlign: 'center', padding: '30px', color: 'var(--muted)' }}>
                  Calculating shortest Dijkstra routes and filtering capacities...
                </div>
              ) : !recommendationData || !recommendationData.found ? (
                <div style={{ textAlign: 'center', padding: '24px', color: 'var(--critical)' }}>
                  ⚠️ {recommendationData?.message || 'No available or reachable shelters found.'}
                </div>
              ) : (
                <div>
                  {/* Primary Recommended Shelter */}
                  <div
                    style={{
                      background: 'rgba(56, 189, 248, 0.08)',
                      border: '1px solid rgba(56, 189, 248, 0.4)',
                      borderRadius: '8px',
                      padding: '16px',
                      marginBottom: '16px',
                    }}
                  >
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                      <span
                        style={{
                          fontSize: '11px',
                          fontWeight: 700,
                          textTransform: 'uppercase',
                          color: '#38BDF8',
                          letterSpacing: '0.04em',
                        }}
                      >
                        🌟 Primary Recommendation
                      </span>
                      <span className="status-pill success">
                        {recommendationData.primary_recommendation.shelter.available_capacity} Spots Open
                      </span>
                    </div>

                    <h3 style={{ fontSize: '17px', color: 'var(--text)', margin: '6px 0 4px 0' }}>
                      {recommendationData.primary_recommendation.shelter.name}
                    </h3>

                    <div style={{ fontSize: '12.5px', color: 'var(--muted)' }}>
                      {recommendationData.primary_recommendation.reason}
                    </div>

                    {/* Dijkstra Route Visualization */}
                    <div
                      style={{
                        background: 'var(--bg)',
                        border: '1px solid var(--line)',
                        borderRadius: '6px',
                        padding: '10px 12px',
                        margin: '12px 0',
                        fontSize: '12px',
                      }}
                    >
                      <div style={{ color: 'var(--muted)', marginBottom: '4px' }}>
                        🛣️ Dijkstra Shortest Route ({recommendationData.primary_recommendation.distance_km} km):
                      </div>
                      <div style={{ display: 'flex', flexWrap: 'wrap', alignItems: 'center', gap: '6px' }}>
                        {recommendationData.primary_recommendation.path_node_names.map((node, idx) => (
                          <span key={idx} style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
                            <strong style={{ color: idx === 0 ? 'var(--warning)' : idx === recommendationData.primary_recommendation.path_node_names.length - 1 ? 'var(--success)' : 'var(--text)' }}>
                              {node}
                            </strong>
                            {idx < recommendationData.primary_recommendation.path_node_names.length - 1 && (
                              <span style={{ color: 'var(--muted)' }}>→</span>
                            )}
                          </span>
                        ))}
                      </div>
                    </div>

                    <button
                      className="btn btn-primary"
                      style={{ width: '100%', marginTop: '4px' }}
                      onClick={() =>
                        handleAssignVictim(
                          selectedVictim.id,
                          recommendationData.primary_recommendation.shelter.id
                        )
                      }
                    >
                      Assign to {recommendationData.primary_recommendation.shelter.name}
                    </button>
                  </div>

                  {/* Alternative Shelters List */}
                  {recommendationData.alternatives?.length > 0 && (
                    <div>
                      <h4 style={{ fontSize: '13px', color: 'var(--muted)', marginBottom: '8px' }}>
                        Alternative Shelters:
                      </h4>
                      <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                        {recommendationData.alternatives.map((alt) => (
                          <div
                            key={alt.shelter.id}
                            style={{
                              background: 'var(--bg)',
                              border: '1px solid var(--line)',
                              borderRadius: '6px',
                              padding: '10px 12px',
                              display: 'flex',
                              justifyContent: 'space-between',
                              alignItems: 'center',
                            }}
                          >
                            <div>
                              <div style={{ fontWeight: 600, color: 'var(--text)', fontSize: '13px' }}>
                                {alt.shelter.name}
                              </div>
                              <div style={{ fontSize: '11.5px', color: 'var(--muted)' }}>
                                {alt.distance_km} km away • {alt.shelter.available_capacity} spots
                              </div>
                            </div>
                            <button
                              className="btn btn-secondary"
                              style={{ padding: '4px 10px', fontSize: '11.5px' }}
                              onClick={() => handleAssignVictim(selectedVictim.id, alt.shelter.id)}
                            >
                              Assign
                            </button>
                          </div>
                        ))}
                      </div>
                    </div>
                  )}
                </div>
              )}
            </div>

            <div className="modal-footer">
              <button type="button" className="btn btn-secondary" onClick={() => setShowRecommendModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 2: REGISTER VICTIM MODAL
          ========================================================================= */}
      {showAddVictimModal && (
        <div className="modal-backdrop" onClick={() => setShowAddVictimModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>+ Register Evacuee / Victim</h3>
              <button className="modal-close" onClick={() => setShowAddVictimModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleCreateVictim}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Victim Name or Identifier *</label>
                  <input
                    type="text"
                    required
                    placeholder="e.g. Jaswinder Kaur"
                    value={victimForm.name}
                    onChange={(e) => setVictimForm({ ...victimForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Age *</label>
                    <input
                      type="number"
                      min="0"
                      max="120"
                      required
                      value={victimForm.age}
                      onChange={(e) => setVictimForm({ ...victimForm, age: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Gender</label>
                    <select
                      value={victimForm.gender}
                      onChange={(e) => setVictimForm({ ...victimForm, gender: e.target.value })}
                    >
                      <option value="F">Female</option>
                      <option value="M">Male</option>
                      <option value="O">Other / Anonymous</option>
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Family Members Accompanying</label>
                    <input
                      type="number"
                      min="0"
                      max="20"
                      value={victimForm.accompanying_people}
                      onChange={(e) => setVictimForm({ ...victimForm, accompanying_people: e.target.value })}
                    />
                  </div>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Emergency Category *</label>
                    <select
                      value={victimForm.emergency_category}
                      onChange={(e) => setVictimForm({ ...victimForm, emergency_category: e.target.value })}
                    >
                      {CATEGORIES.filter((c) => c !== 'All').map((c) => (
                        <option key={c} value={c}>
                          {c}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Current Location *</label>
                    <select
                      value={victimForm.current_location_id}
                      onChange={(e) => setVictimForm({ ...victimForm, current_location_id: e.target.value })}
                    >
                      {locations.map((loc) => (
                        <option key={loc.id} value={loc.id}>
                          {loc.name}
                        </option>
                      ))}
                    </select>
                  </div>
                </div>

                <div className="form-group">
                  <label style={{ display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer' }}>
                    <input
                      type="checkbox"
                      checked={victimForm.medical_assistance}
                      onChange={(e) => setVictimForm({ ...victimForm, medical_assistance: e.target.checked })}
                    />
                    <span>Requires urgent medical assistance / intensive care</span>
                  </label>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowAddVictimModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Register Victim
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 3: EDIT VICTIM MODAL
          ========================================================================= */}
      {showEditVictimModal && selectedVictim && (
        <div className="modal-backdrop" onClick={() => setShowEditVictimModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>✏️ Edit Victim #{selectedVictim.id}</h3>
              <button className="modal-close" onClick={() => setShowEditVictimModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleUpdateVictim}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Victim Name *</label>
                  <input
                    type="text"
                    required
                    value={victimForm.name}
                    onChange={(e) => setVictimForm({ ...victimForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Age</label>
                    <input
                      type="number"
                      min="0"
                      max="120"
                      required
                      value={victimForm.age}
                      onChange={(e) => setVictimForm({ ...victimForm, age: e.target.value })}
                    />
                  </div>
                  <div className="form-group">
                    <label>Family Members Accompanying</label>
                    <input
                      type="number"
                      min="0"
                      max="20"
                      value={victimForm.accompanying_people}
                      onChange={(e) => setVictimForm({ ...victimForm, accompanying_people: e.target.value })}
                    />
                  </div>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Emergency Category</label>
                    <select
                      value={victimForm.emergency_category}
                      onChange={(e) => setVictimForm({ ...victimForm, emergency_category: e.target.value })}
                    >
                      {CATEGORIES.filter((c) => c !== 'All').map((c) => (
                        <option key={c} value={c}>
                          {c}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Current Location</label>
                    <select
                      value={victimForm.current_location_id}
                      onChange={(e) => setVictimForm({ ...victimForm, current_location_id: e.target.value })}
                    >
                      {locations.map((loc) => (
                        <option key={loc.id} value={loc.id}>
                          {loc.name}
                        </option>
                      ))}
                    </select>
                  </div>
                </div>

                <div className="form-group">
                  <label>Assigned Shelter</label>
                  <select
                    value={victimForm.assigned_shelter_id}
                    onChange={(e) => setVictimForm({ ...victimForm, assigned_shelter_id: e.target.value })}
                  >
                    <option value="">Awaiting Shelter (Unassigned)</option>
                    {shelters.map((s) => (
                      <option key={s.id} value={s.id}>
                        {s.name} ({s.available_capacity} spots open)
                      </option>
                    ))}
                  </select>
                </div>

                <div className="form-group">
                  <label style={{ display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer' }}>
                    <input
                      type="checkbox"
                      checked={victimForm.medical_assistance}
                      onChange={(e) => setVictimForm({ ...victimForm, medical_assistance: e.target.checked })}
                    />
                    <span>Requires urgent medical assistance</span>
                  </label>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowEditVictimModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Save Changes
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 4: ADD SHELTER MODAL
          ========================================================================= */}
      {showAddShelterModal && (
        <div className="modal-backdrop" onClick={() => setShowAddShelterModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>+ Open New Emergency Shelter</h3>
              <button className="modal-close" onClick={() => setShowAddShelterModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleCreateShelter}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Shelter Name *</label>
                  <input
                    type="text"
                    required
                    placeholder="e.g. West District Relief Hub"
                    value={shelterForm.name}
                    onChange={(e) => setShelterForm({ ...shelterForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Location *</label>
                    <select
                      value={shelterForm.location_id}
                      onChange={(e) => setShelterForm({ ...shelterForm, location_id: e.target.value })}
                    >
                      {locations.map((loc) => (
                        <option key={loc.id} value={loc.id}>
                          {loc.name}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Total Capacity (Beds) *</label>
                    <input
                      type="number"
                      min="1"
                      required
                      value={shelterForm.total_capacity}
                      onChange={(e) => setShelterForm({ ...shelterForm, total_capacity: e.target.value })}
                    />
                  </div>
                </div>

                <div className="form-group">
                  <label>Available Facilities</label>
                  <input
                    type="text"
                    placeholder="e.g. Clean Water, Food Stock, Sanitation, Backup Power"
                    value={shelterForm.facilities}
                    onChange={(e) => setShelterForm({ ...shelterForm, facilities: e.target.value })}
                  />
                </div>

                <div className="form-group">
                  <label style={{ display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer' }}>
                    <input
                      type="checkbox"
                      checked={shelterForm.has_medical}
                      onChange={(e) => setShelterForm({ ...shelterForm, has_medical: e.target.checked })}
                    />
                    <span>Equipped with On-Site Medical Bay</span>
                  </label>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowAddShelterModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Create Shelter
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =========================================================================
          MODAL 5: EDIT SHELTER MODAL
          ========================================================================= */}
      {showEditShelterModal && selectedShelter && (
        <div className="modal-backdrop" onClick={() => setShowEditShelterModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>✏️ Edit Shelter #{selectedShelter.id}</h3>
              <button className="modal-close" onClick={() => setShowEditShelterModal(false)}>
                ×
              </button>
            </div>
            <form onSubmit={handleUpdateShelter}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Shelter Name *</label>
                  <input
                    type="text"
                    required
                    value={shelterForm.name}
                    onChange={(e) => setShelterForm({ ...shelterForm, name: e.target.value })}
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Location</label>
                    <select
                      value={shelterForm.location_id}
                      onChange={(e) => setShelterForm({ ...shelterForm, location_id: e.target.value })}
                    >
                      {locations.map((loc) => (
                        <option key={loc.id} value={loc.id}>
                          {loc.name}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Total Capacity (Beds) *</label>
                    <input
                      type="number"
                      min={selectedShelter.current_occupancy}
                      required
                      value={shelterForm.total_capacity}
                      onChange={(e) => setShelterForm({ ...shelterForm, total_capacity: e.target.value })}
                    />
                    <small style={{ color: 'var(--muted)', fontSize: '11px', display: 'block', marginTop: '2px' }}>
                      Current occupancy is {selectedShelter.current_occupancy}. Cannot set capacity lower.
                    </small>
                  </div>
                </div>

                <div className="form-group">
                  <label>Facilities</label>
                  <input
                    type="text"
                    value={shelterForm.facilities}
                    onChange={(e) => setShelterForm({ ...shelterForm, facilities: e.target.value })}
                  />
                </div>

                <div className="form-group">
                  <label style={{ display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer' }}>
                    <input
                      type="checkbox"
                      checked={shelterForm.has_medical}
                      onChange={(e) => setShelterForm({ ...shelterForm, has_medical: e.target.checked })}
                    />
                    <span>Equipped with Medical Bay</span>
                  </label>
                </div>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn btn-secondary" onClick={() => setShowEditShelterModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn btn-primary">
                  Save Changes
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}
