import { useCallback, useEffect, useState } from 'react';
import { getLocations } from '../services/mapService.js';
import {
  createRequest,
  dispatchTeam,
  getRescueHeap,
  getRescueOverview,
  getRequests,
  getRequest,
  getTeams,
  optimizeAllocation,
  updateRequestStatus,
} from '../services/rescueService.js';

const SEVERITIES = ['Critical', 'High', 'Medium', 'Low'];
const EMERGENCY_TYPES = ['Fire', 'Flood', 'Medical', 'Accident', 'Building Collapse', 'Other'];
const STATUSES = [
  { id: 'pending', label: 'Pending' },
  { id: 'assigned', label: 'Assigned' },
  { id: 'in_progress', label: 'In Progress' },
  { id: 'completed', label: 'Completed' },
];

export default function RescueRequests() {
  const [requests, setRequests] = useState([]);
  const [overview, setOverview] = useState({
    total: 0,
    critical: 0,
    pending: 0,
    assigned: 0,
    completed: 0,
    available_teams: 0,
    total_teams: 0,
  });
  const [locations, setLocations] = useState([]);
  const [teams, setTeams] = useState([]);

  // Filtering & Sorting State
  const [search, setSearch] = useState('');
  const [filterSeverity, setFilterSeverity] = useState('All');
  const [filterStatus, setFilterStatus] = useState('All');
  const [filterType, setFilterType] = useState('All');
  const [sortBy, setSortBy] = useState('priority');
  const [sortOrder, setSortOrder] = useState('desc');

  // Loading & Toast State
  const [loading, setLoading] = useState(false);
  const [toast, setToast] = useState(null); // { text, type: 'success'|'error'|'info' }

  // Modal States
  const [showAddModal, setShowAddModal] = useState(false);
  const [showDetailModal, setShowDetailModal] = useState(false);
  const [showDispatchModal, setShowDispatchModal] = useState(false);
  const [showDpModal, setShowDpModal] = useState(false);
  const [showHeapModal, setShowHeapModal] = useState(false);

  // Active Request / Detail Data
  const [selectedRequest, setSelectedRequest] = useState(null);
  const [detailData, setDetailData] = useState(null);
  const [dispatchResult, setDispatchResult] = useState(null);
  const [selectedTeamId, setSelectedTeamId] = useState('');

  // DP Optimizer Data
  const [dpResult, setDpResult] = useState(null);
  const [customCapacity, setCustomCapacity] = useState('');

  // Heap Visualizer Data
  const [heapData, setHeapData] = useState([]);

  // Form State
  const [formData, setFormData] = useState({
    victim_name: '',
    emergency_type: 'Flood',
    severity: 'High',
    affected_people: 1,
    location_id: '',
    description: '',
    required_assistance: '',
  });
  const [formErrors, setFormErrors] = useState({});

  const showNotification = (text, type = 'success') => {
    setToast({ text, type });
    setTimeout(() => setToast(null), 6000);
  };

  const loadAllData = useCallback(async () => {
    setLoading(true);
    try {
      const [reqs, ov, locs, tm] = await Promise.all([
        getRequests({
          sort: sortBy,
          order: sortOrder,
          severity: filterSeverity,
          status: filterStatus,
          emergency_type: filterType,
          search: search.trim(),
        }),
        getRescueOverview(),
        getLocations(),
        getTeams(),
      ]);
      setRequests(reqs);
      setOverview(ov);
      setLocations(locs);
      setTeams(tm);
    } catch (e) {
      showNotification(e.message, 'error');
    } finally {
      setLoading(false);
    }
  }, [sortBy, sortOrder, filterSeverity, filterStatus, filterType, search]);

  useEffect(() => {
    loadAllData();
  }, [loadAllData]);

  // Handle Form Validation
  const validateForm = () => {
    const errors = {};
    if (!formData.victim_name.trim()) {
      errors.victim_name = 'Victim identifier or name is required';
    }
    if (!formData.location_id) {
      errors.location_id = 'Please select a location';
    }
    if (!formData.affected_people || Number(formData.affected_people) <= 0) {
      errors.affected_people = 'Affected people must be greater than 0';
    }
    if (!formData.description.trim()) {
      errors.description = 'Brief emergency description is required';
    }
    setFormErrors(errors);
    return Object.keys(errors).length === 0;
  };

  // Submit New Request
  const handleCreateRequest = async (e) => {
    e.preventDefault();
    if (!validateForm()) return;

    try {
      const payload = {
        victim_name: formData.victim_name.trim(),
        emergency_type: formData.emergency_type,
        severity: formData.severity,
        affected_people: Number(formData.affected_people),
        location_id: Number(formData.location_id),
        description: formData.description.trim(),
        required_assistance: formData.required_assistance.trim(),
      };
      const created = await createRequest(payload);
      showNotification(`Emergency Request #${created.id} logged successfully with Priority Score ${created.priority_score}.`, 'success');
      setShowAddModal(false);
      setFormData({
        victim_name: '',
        emergency_type: 'Flood',
        severity: 'High',
        affected_people: 1,
        location_id: '',
        description: '',
        required_assistance: '',
      });
      loadAllData();
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Open Details Modal
  const openDetails = async (req) => {
    setSelectedRequest(req);
    setShowDetailModal(true);
    setDetailData(null);
    try {
      const d = await getRequest(req.id);
      setDetailData(d);
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Update Status
  const handleStatusChange = async (reqId, newStatus) => {
    try {
      const updated = await updateRequestStatus(reqId, newStatus);
      showNotification(`Request #${reqId} status updated to ${updated.status}.`, 'info');
      if (detailData && detailData.id === reqId) {
        setDetailData((prev) => ({ ...prev, status: updated.status }));
      }
      loadAllData();
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Open Dispatch Modal
  const openDispatch = (req) => {
    setSelectedRequest(req);
    setDispatchResult(null);
    setSelectedTeamId('');
    setShowDispatchModal(true);
  };

  // Execute Dispatch
  const handleDispatch = async () => {
    if (!selectedRequest) return;
    try {
      const res = await dispatchTeam(selectedRequest.id, selectedTeamId ? Number(selectedTeamId) : null);
      setDispatchResult(res);
      showNotification(res.message, 'success');
      loadAllData();
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Run DP Optimizer
  const handleRunOptimizer = async () => {
    try {
      const res = await optimizeAllocation(customCapacity ? Number(customCapacity) : null);
      setDpResult(res);
      setShowDpModal(true);
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Open Max Heap View
  const handleOpenHeap = async () => {
    try {
      const res = await getRescueHeap();
      setHeapData(res.heap || []);
      setShowHeapModal(true);
    } catch (err) {
      showNotification(err.message, 'error');
    }
  };

  // Helpers for styling badges
  const getSeverityBadgeClass = (sev, status) => {
    if (status === 'completed' || status === 'resolved') return 'badge completed';
    const s = String(sev).toLowerCase();
    if (s === 'critical' || s === '5' || s === '4') return 'badge critical';
    if (s === 'high' || s === '3') return 'badge high';
    if (s === 'medium' || s === '2') return 'badge medium';
    return 'badge low';
  };

  const getStatusBadgeClass = (status) => {
    switch (status) {
      case 'completed':
      case 'resolved':
        return 'badge completed';
      case 'assigned':
        return 'badge assigned';
      case 'in_progress':
        return 'badge in_progress';
      case 'pending':
      default:
        return 'badge pending';
    }
  };

  const getEmergencyIcon = (type) => {
    switch (type) {
      case 'Fire': return '🔥';
      case 'Flood': return '🌊';
      case 'Medical': return '🏥';
      case 'Accident': return '🚗';
      case 'Building Collapse': return '🏚️';
      default: return '⚠️';
    }
  };

  const nextIdPreview = overview.total > 0 ? overview.total + 1 : 1;

  return (
    <>
      {/* 1. PAGE HEADER */}
      <header className="page-head">
        <h1>Rescue Requests</h1>
        <p>Log emergencies, rank them by urgency and send the nearest available team.</p>
      </header>

      {/* Global Toast Notification */}
      {toast && (
        <div className={`toast-banner ${toast.type}`} role="alert">
          <span>{toast.text}</span>
          <button className="toast-close" onClick={() => setToast(null)}>×</button>
        </div>
      )}

      {/* 3. REQUEST OVERVIEW SUMMARY CARDS */}
      <section className="stats" aria-label="Rescue request summary cards">
        <div className="card stat">
          <span className="stat-label">Total Requests</span>
          <strong className="tone-primary">{overview.total}</strong>
          <span className="stat-note">All emergencies logged</span>
        </div>
        <div className="card stat">
          <span className="stat-label">Critical Requests</span>
          <strong className="tone-critical">{overview.critical}</strong>
          <span className="stat-note">Life-threatening priority</span>
        </div>
        <div className="card stat">
          <span className="stat-label">Pending Requests</span>
          <strong className="tone-warning">{overview.pending}</strong>
          <span className="stat-note">Awaiting team dispatch</span>
        </div>
        <div className="card stat">
          <span className="stat-label">Assigned Requests</span>
          <strong style={{ color: '#C084FC' }}>{overview.assigned}</strong>
          <span className="stat-note">{overview.available_teams} of {overview.total_teams} teams ready</span>
        </div>
      </section>

      {/* Main Section */}
      <section className="card" style={{ marginBottom: '24px' }}>
        {/* Action Toolbar */}
        <div className="toolbar">
          <div>
            <h3>Emergency Management & Dispatch</h3>
            <p className="muted">Max heap prioritization with Dijkstra nearest-team routing.</p>
          </div>
          <div className="toolbar-actions">
            <button className="btn-secondary" onClick={handleOpenHeap} title="View live C++ binary max-heap">
              📊 Max Heap View
            </button>
            <button className="btn-secondary" onClick={handleRunOptimizer} title="Run Dynamic Programming Knapsack allocation">
              ⚡ DP Optimizer
            </button>
            <button className="btn" onClick={() => setShowAddModal(true)}>
              + Add Emergency Request
            </button>
          </div>
        </div>

        {/* 4. SEARCH & FILTERING CONTROLS */}
        <div className="filter-bar">
          <label>
            Search
            <input
              type="text"
              placeholder="Search victim, location, or #ID..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
            />
          </label>
          <label>
            Severity
            <select value={filterSeverity} onChange={(e) => setFilterSeverity(e.target.value)}>
              <option value="All">All Severities</option>
              {SEVERITIES.map((s) => (
                <option key={s} value={s}>{s}</option>
              ))}
            </select>
          </label>
          <label>
            Status
            <select value={filterStatus} onChange={(e) => setFilterStatus(e.target.value)}>
              <option value="All">All Statuses</option>
              {STATUSES.map((st) => (
                <option key={st.id} value={st.id}>{st.label}</option>
              ))}
            </select>
          </label>
          <label>
            Emergency Type
            <select value={filterType} onChange={(e) => setFilterType(e.target.value)}>
              <option value="All">All Types</option>
              {EMERGENCY_TYPES.map((t) => (
                <option key={t} value={t}>{t}</option>
              ))}
            </select>
          </label>
          <label>
            Sort By
            <select value={sortBy} onChange={(e) => setSortBy(e.target.value)}>
              <option value="priority">Priority Score (Merge Sort)</option>
              <option value="time">Waiting Time</option>
              <option value="severity">Severity</option>
              <option value="people">People Affected</option>
              <option value="id">Request ID</option>
            </select>
          </label>
        </div>

        {/* 4. PRIORITY-BASED REQUEST TABLE */}
        <div className="table-wrap">
          <table className="rescue-table">
            <thead>
              <tr>
                <th>Request ID</th>
                <th>Emergency Type</th>
                <th>Location</th>
                <th>Severity</th>
                <th>People Affected</th>
                <th>Priority Score</th>
                <th>Assigned Team</th>
                <th>Status</th>
                <th style={{ textAlign: 'right' }}>Action</th>
              </tr>
            </thead>
            <tbody>
              {loading && requests.length === 0 ? (
                <tr>
                  <td colSpan="9" style={{ textAlign: 'center', padding: '36px', color: 'var(--muted)' }}>
                    Loading emergency requests from C++ engine...
                  </td>
                </tr>
              ) : requests.length === 0 ? (
                <tr>
                  <td colSpan="9" style={{ textAlign: 'center', padding: '36px', color: 'var(--muted)' }}>
                    No emergency requests match the selected filters.
                  </td>
                </tr>
              ) : (
                requests.map((r) => {
                  const isCrit = r.severity === 'Critical' && r.status === 'pending';
                  const isHigh = r.severity === 'High' && r.status === 'pending';
                  const scoreWidth = Math.min(100, Math.max(8, r.priority_score));
                  return (
                    <tr
                      key={r.id}
                      className={isCrit ? 'critical-row' : isHigh ? 'high-row' : ''}
                    >
                      <td>
                        <strong>#{r.id}</strong>
                      </td>
                      <td>
                        <span style={{ marginRight: '6px' }}>{getEmergencyIcon(r.emergency_type)}</span>
                        {r.emergency_type}
                      </td>
                      <td>{r.location_name}</td>
                      <td>
                        <span className={getSeverityBadgeClass(r.severity, r.status)}>
                          {r.severity}
                        </span>
                      </td>
                      <td>
                        <strong>{r.affected_people}</strong>
                        <span className="muted" style={{ marginLeft: '4px' }}>people</span>
                      </td>
                      <td>
                        <div className="priority-cell">
                          <span className="score-pill">{r.priority_score}</span>
                          <div className="score-track" title={`Priority score: ${r.priority_score}`}>
                            <div
                              className="score-fill"
                              style={{
                                width: `${scoreWidth}%`,
                                background: r.priority_score >= 80 ? 'var(--critical)' : r.priority_score >= 60 ? 'var(--warning)' : 'var(--primary)',
                              }}
                            />
                          </div>
                        </div>
                      </td>
                      <td>
                        {r.assigned_team_name ? (
                          <span style={{ color: 'var(--text)', fontWeight: 500 }}>
                            {r.assigned_team_name}
                          </span>
                        ) : (
                          <span className="muted">None</span>
                        )}
                      </td>
                      <td>
                        <span className={getStatusBadgeClass(r.status)}>
                          {r.status === 'in_progress' ? 'In Progress' : r.status.charAt(0).toUpperCase() + r.status.slice(1)}
                        </span>
                      </td>
                      <td>
                        <div className="action-group" style={{ justifyContent: 'flex-end' }}>
                          {r.status === 'pending' && (
                            <button
                              className="btn-action-dispatch"
                              onClick={() => openDispatch(r)}
                              title="Calculate shortest route and dispatch nearest suitable team"
                            >
                              Dispatch
                            </button>
                          )}
                          <button
                            className="btn-action-details"
                            onClick={() => openDetails(r)}
                            title="View request details and priority score breakdown"
                          >
                            Details
                          </button>
                        </div>
                      </td>
                    </tr>
                  );
                })
              )}
            </tbody>
          </table>
        </div>
      </section>

      {/* 2. RESCUE REQUEST MODAL FORM */}
      {showAddModal && (
        <div className="modal-backdrop" onClick={() => setShowAddModal(false)}>
          <div className="modal-window" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h2>Add Emergency Request</h2>
              <button className="modal-close" onClick={() => setShowAddModal(false)}>×</button>
            </div>
            <form onSubmit={handleCreateRequest}>
              <div className="modal-body">
                <p className="muted">
                  Log a new emergency incident. The C++ engine will calculate its priority score using severity, victim count, and arrival time.
                </p>

                <div className="form-grid-2">
                  <label>
                    Request ID (Auto)
                    <input type="text" disabled value={`#${nextIdPreview} (Auto-assigned)`} />
                  </label>
                  <label>
                    Time Received (Auto)
                    <input type="text" disabled value="Just now (System timestamp)" />
                  </label>
                </div>

                <div className="form-grid-2">
                  <label>
                    Victim Name / Identifier *
                    <input
                      type="text"
                      placeholder="e.g. Gurpreet Singh or Anonymous-01"
                      value={formData.victim_name}
                      onChange={(e) => setFormData({ ...formData, victim_name: e.target.value })}
                      required
                    />
                    {formErrors.victim_name && <span className="form-err">{formErrors.victim_name}</span>}
                  </label>
                  <label>
                    Emergency Type *
                    <select
                      value={formData.emergency_type}
                      onChange={(e) => setFormData({ ...formData, emergency_type: e.target.value })}
                    >
                      {EMERGENCY_TYPES.map((t) => (
                        <option key={t} value={t}>{t}</option>
                      ))}
                    </select>
                  </label>
                </div>

                <div className="form-grid-2">
                  <label>
                    Severity Level *
                    <select
                      value={formData.severity}
                      onChange={(e) => setFormData({ ...formData, severity: e.target.value })}
                    >
                      {SEVERITIES.map((s) => (
                        <option key={s} value={s}>{s}</option>
                      ))}
                    </select>
                  </label>
                  <label>
                    Number of Affected People *
                    <input
                      type="number"
                      min="1"
                      max="10000"
                      value={formData.affected_people}
                      onChange={(e) => setFormData({ ...formData, affected_people: e.target.value })}
                      required
                    />
                    {formErrors.affected_people && <span className="form-err">{formErrors.affected_people}</span>}
                  </label>
                </div>

                <label>
                  Location *
                  <select
                    value={formData.location_id}
                    onChange={(e) => setFormData({ ...formData, location_id: e.target.value })}
                    required
                  >
                    <option value="">Select emergency location</option>
                    {locations.map((loc) => (
                      <option key={loc.id} value={loc.id}>
                        {loc.name} ({loc.type})
                      </option>
                    ))}
                  </select>
                  {formErrors.location_id && <span className="form-err">{formErrors.location_id}</span>}
                </label>

                <label>
                  Description *
                  <textarea
                    placeholder="Describe the emergency conditions, hazards, or trapped status..."
                    value={formData.description}
                    onChange={(e) => setFormData({ ...formData, description: e.target.value })}
                    required
                  />
                  {formErrors.description && <span className="form-err">{formErrors.description}</span>}
                </label>

                <label>
                  Required Assistance
                  <input
                    type="text"
                    placeholder="e.g. Life jackets, inflatable boat, ambulance, rescue crane"
                    value={formData.required_assistance}
                    onChange={(e) => setFormData({ ...formData, required_assistance: e.target.value })}
                  />
                </label>
              </div>

              <div className="modal-footer">
                <button type="button" className="btn-secondary" onClick={() => setShowAddModal(false)}>
                  Cancel
                </button>
                <button type="submit" className="btn">
                  Submit Emergency Request
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* 7. REQUEST DETAILS MODAL */}
      {showDetailModal && selectedRequest && (
        <div className="modal-backdrop" onClick={() => setShowDetailModal(false)}>
          <div className="modal-window wide" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h2>Request Details — #{selectedRequest.id}</h2>
              <button className="modal-close" onClick={() => setShowDetailModal(false)}>×</button>
            </div>
            <div className="modal-body">
              <div className="form-grid-2">
                <div>
                  <h4 style={{ marginBottom: '6px' }}>Incident Information</h4>
                  <p><strong>Victim:</strong> {selectedRequest.victim_name}</p>
                  <p><strong>Emergency:</strong> {selectedRequest.emergency_type} ({selectedRequest.severity})</p>
                  <p><strong>Location:</strong> {selectedRequest.location_name}</p>
                  <p><strong>Affected Count:</strong> {selectedRequest.affected_people} individuals</p>
                  <p><strong>Time Reported:</strong> {selectedRequest.created_at}</p>
                  <p><strong>Waiting Time:</strong> {selectedRequest.waiting_minutes} minutes</p>
                </div>
                <div>
                  <h4 style={{ marginBottom: '6px' }}>Current Status & Lifecycle</h4>
                  <label>Update Status
                    <select
                      value={detailData ? detailData.status : selectedRequest.status}
                      onChange={(e) => handleStatusChange(selectedRequest.id, e.target.value)}
                    >
                      <option value="pending">Pending</option>
                      <option value="assigned">Assigned</option>
                      <option value="in_progress">In Progress</option>
                      <option value="completed">Completed</option>
                    </select>
                  </label>
                  <p className="muted" style={{ marginTop: '8px' }}>
                    Marking an emergency as <em>Completed</em> automatically releases the deployed rescue team back to available status.
                  </p>
                </div>
              </div>

              {/* Priority Formula Breakdown */}
              <div style={{ marginTop: '12px' }}>
                <h4>C++ Priority Score Formula Breakdown</h4>
                <p className="muted">
                  Score = Severity Base + min(30, People × 1.5) + min(20, Wait Mins × 0.2) + Type Bonus
                </p>
                {detailData && detailData.priority_breakdown ? (
                  <div className="breakdown-grid">
                    <div className="breakdown-card">
                      <span>Severity ({selectedRequest.severity})</span>
                      <strong>{detailData.priority_breakdown.severity_component}</strong>
                    </div>
                    <div className="breakdown-card">
                      <span>Victims ({selectedRequest.affected_people} people)</span>
                      <strong>{detailData.priority_breakdown.people_component}</strong>
                    </div>
                    <div className="breakdown-card">
                      <span>Waiting ({selectedRequest.waiting_minutes}m)</span>
                      <strong>{detailData.priority_breakdown.waiting_component}</strong>
                    </div>
                    <div className="breakdown-card">
                      <span>{selectedRequest.emergency_type} Bonus</span>
                      <strong>+{detailData.priority_breakdown.type_bonus}</strong>
                    </div>
                  </div>
                ) : (
                  <div className="breakdown-card" style={{ marginTop: '8px' }}>
                    <strong>{selectedRequest.priority_score} Total Priority Score</strong>
                  </div>
                )}
              </div>

              {/* Description & Assistance */}
              <div>
                <h4>Description</h4>
                <p style={{ background: 'var(--bg)', padding: '10px', borderRadius: '6px', border: '1px solid var(--line)' }}>
                  {selectedRequest.description || 'No detailed description provided.'}
                </p>
                {selectedRequest.required_assistance && (
                  <p style={{ marginTop: '8px' }}>
                    <strong>Required Assistance:</strong> {selectedRequest.required_assistance}
                  </p>
                )}
              </div>

              {/* Assigned Team & Route if Available */}
              {detailData && detailData.assignment && (
                <div style={{ background: 'rgba(56, 189, 248, 0.05)', padding: '14px', borderRadius: '8px', border: '1px solid rgba(56, 189, 248, 0.2)' }}>
                  <h4>Assigned Team & Route Information</h4>
                  <p><strong>Team:</strong> {detailData.team_details ? detailData.team_details.name : selectedRequest.assigned_team_name} ({detailData.team_details ? detailData.team_details.team_type : ''})</p>
                  <p><strong>Route Distance:</strong> {detailData.assignment.route_distance} km</p>
                  <p><strong>Assigned At:</strong> {detailData.assignment.assigned_at}</p>
                </div>
              )}

              {/* Event History */}
              {detailData && detailData.history && detailData.history.length > 0 && (
                <div>
                  <h4>Audit History</h4>
                  <ul className="events">
                    {detailData.history.map((ev) => (
                      <li key={ev.id}>
                        <i className={`dot ${ev.category}`} />
                        <span className="ev-msg">{ev.message}</span>
                        <small className="muted">{ev.created_at}</small>
                      </li>
                    ))}
                  </ul>
                </div>
              )}
            </div>

            <div className="modal-footer">
              {selectedRequest.status === 'pending' && (
                <button
                  className="btn"
                  onClick={() => {
                    setShowDetailModal(false);
                    openDispatch(selectedRequest);
                  }}
                >
                  Dispatch Nearest Team
                </button>
              )}
              <button className="btn-secondary" onClick={() => setShowDetailModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 6. DISPATCH CONFIRMATION & DIJKSTRA MODAL */}
      {showDispatchModal && selectedRequest && (
        <div className="modal-backdrop" onClick={() => setShowDispatchModal(false)}>
          <div className="modal-window wide" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h2>Dispatch Rescue Team — #{selectedRequest.id}</h2>
              <button className="modal-close" onClick={() => setShowDispatchModal(false)}>×</button>
            </div>
            <div className="modal-body">
              <p>
                Dispatching team to <strong>{selectedRequest.victim_name}</strong> at <strong>{selectedRequest.location_name}</strong> ({selectedRequest.emergency_type}, Severity: {selectedRequest.severity}).
              </p>

              <label>
                Rescue Team Selection
                <select
                  value={selectedTeamId}
                  onChange={(e) => {
                    setSelectedTeamId(e.target.value);
                    setDispatchResult(null);
                  }}
                >
                  <option value="">Automatic: Find Nearest Suitable Team (Dijkstra)</option>
                  {teams.filter((t) => t.availability === 'available').map((t) => (
                    <option key={t.id} value={t.id}>
                      {t.name} ({t.team_type.toUpperCase()}) — at {t.location_name} (Cap: {t.capacity})
                    </option>
                  ))}
                </select>
              </label>

              <div style={{ background: 'var(--bg)', padding: '14px', borderRadius: '8px', border: '1px solid var(--line)' }}>
                <h4>Graph & Routing Engine</h4>
                <p className="muted">
                  The C++ engine will execute Dijkstra shortest path algorithm from the team base to {selectedRequest.location_name}, automatically skipping any blocked roads.
                </p>
              </div>

              {/* Calculated Route Presentation */}
              {dispatchResult && dispatchResult.route && (
                <div className="result" style={{ background: 'rgba(34, 197, 94, 0.05)', padding: '16px', borderRadius: '8px', border: '1px solid rgba(34, 197, 94, 0.3)' }}>
                  <h3 style={{ color: 'var(--success)' }}>Team Dispatched Successfully</h3>
                  <div className="result-nums" style={{ marginTop: '10px' }}>
                    <div>
                      <strong>{dispatchResult.team.name}</strong>
                      <span>Assigned Unit</span>
                    </div>
                    <div>
                      <strong>{dispatchResult.route.distance_km} km</strong>
                      <span>Dijkstra Distance</span>
                    </div>
                    <div>
                      <strong>{dispatchResult.route.hops}</strong>
                      <span>Road Segments</span>
                    </div>
                  </div>
                  <p className="muted" style={{ marginTop: '8px' }}>
                    Route computed in {dispatchResult.route.runtime_us} µs.
                  </p>
                  <ol className="path" style={{ marginTop: '8px' }}>
                    {dispatchResult.route.path.map((node) => (
                      <li key={node.id}>
                        <strong>{node.name}</strong>
                      </li>
                    ))}
                  </ol>
                </div>
              )}
            </div>

            <div className="modal-footer">
              <button className="btn-secondary" onClick={() => setShowDispatchModal(false)}>
                {dispatchResult ? 'Done' : 'Cancel'}
              </button>
              {!dispatchResult && (
                <button className="btn" onClick={handleDispatch}>
                  Confirm & Dispatch Team
                </button>
              )}
            </div>
          </div>
        </div>
      )}

      {/* 5F. DYNAMIC PROGRAMMING OPTIMIZER MODAL */}
      {showDpModal && (
        <div className="modal-backdrop" onClick={() => setShowDpModal(false)}>
          <div className="modal-window wide" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h2>Rescue Allocation Optimizer (Dynamic Programming)</h2>
              <button className="modal-close" onClick={() => setShowDpModal(false)}>×</button>
            </div>
            <div className="modal-body">
              <p className="muted">
                Solves a multi-request 0/1 Knapsack optimization problem in C++ to determine which subset of competing emergency requests to fulfill under constrained rescue team capacity to maximize lives saved and priority scores.
              </p>

              <div style={{ display: 'flex', gap: '10px', alignItems: 'flex-end' }}>
                <label style={{ flex: 1 }}>
                  Rescue Capacity Limit (Persons)
                  <input
                    type="number"
                    placeholder="Leave blank for total available team capacity"
                    value={customCapacity}
                    onChange={(e) => setCustomCapacity(e.target.value)}
                  />
                </label>
                <button className="btn" onClick={handleRunOptimizer} style={{ marginBottom: '12px' }}>
                  Re-run DP Solver
                </button>
              </div>

              {dpResult && (
                <>
                  <div className="stats" style={{ marginTop: '12px', gridTemplateColumns: 'repeat(4, 1fr)' }}>
                    <div className="card stat">
                      <span className="stat-label">Capacity Budget</span>
                      <strong>{dpResult.total_capacity}</strong>
                      <span className="stat-note">{dpResult.used_capacity} used</span>
                    </div>
                    <div className="card stat">
                      <span className="stat-label">Optimized Score</span>
                      <strong className="tone-primary">{dpResult.total_priority_score}</strong>
                      <span className="stat-note">Maximized via DP</span>
                    </div>
                    <div className="card stat">
                      <span className="stat-label">Lives Saved</span>
                      <strong className="tone-success">{dpResult.total_people_saved}</strong>
                      <span className="stat-note">{dpResult.selected_count} requests</span>
                    </div>
                    <div className="card stat">
                      <span className="stat-label">C++ Runtime</span>
                      <strong>{dpResult.runtime_us} µs</strong>
                      <span className="stat-note">O(N × Capacity)</span>
                    </div>
                  </div>

                  <h4>Prioritized Requests in Optimal Subset</h4>
                  <div className="table-wrap">
                    <table className="rescue-table">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Victim</th>
                          <th>Emergency</th>
                          <th>Location</th>
                          <th>People</th>
                          <th>Priority Score</th>
                        </tr>
                      </thead>
                      <tbody>
                        {dpResult.selected_requests.map((r) => (
                          <tr key={r.id}>
                            <td><strong>#{r.id}</strong></td>
                            <td>{r.victim_name}</td>
                            <td>{getEmergencyIcon(r.emergency_type)} {r.emergency_type}</td>
                            <td>{r.location_name}</td>
                            <td><strong>{r.affected_people}</strong></td>
                            <td><span className="score-pill">{r.priority_score}</span></td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                </>
              )}
            </div>

            <div className="modal-footer">
              <button className="btn-secondary" onClick={() => setShowDpModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 5A. MAX HEAP VISUALIZER MODAL */}
      {showHeapModal && (
        <div className="modal-backdrop" onClick={() => setShowHeapModal(false)}>
          <div className="modal-window wide" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h2>C++ Binary Max-Heap Inspector</h2>
              <button className="modal-close" onClick={() => setShowHeapModal(false)}>×</button>
            </div>
            <div className="modal-body">
              <p className="muted">
                Live array representation of the binary Max-Heap maintaining pending emergency requests. The root element at Index 0 is always guaranteed to hold the highest urgency score.
              </p>

              {heapData.length === 0 ? (
                <p className="muted" style={{ textAlign: 'center', padding: '24px' }}>
                  No pending requests currently in the heap.
                </p>
              ) : (
                <div className="heap-container">
                  {heapData.map((node, idx) => (
                    <div key={node.id} className={`heap-node ${idx === 0 ? 'root' : ''}`}>
                      <div className="node-idx">{idx === 0 ? '👑 Root [0]' : `Node [${idx}]`}</div>
                      <div className="node-score">{node.priority_score}</div>
                      <div className="node-label">
                        <strong>#{node.id}</strong> {node.victim_name}
                      </div>
                      <small className="muted">{node.emergency_type} • {node.location_name}</small>
                    </div>
                  ))}
                </div>
              )}
            </div>

            <div className="modal-footer">
              <button className="btn-secondary" onClick={() => setShowHeapModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
