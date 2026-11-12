import { NavLink } from 'react-router-dom';

const links = [
  { to: '/', label: 'Dashboard', icon: 'M3 3h7v9H3zM14 3h7v5h-7zM14 12h7v9h-7zM3 16h7v5H3z' },
  { to: '/map', label: 'Disaster Map', icon: 'M9 4 3 6v14l6-2 6 2 6-2V4l-6 2zM9 4v14M15 6v14' },
  { to: '/requests', label: 'Rescue Requests', icon: 'M12 3 2 20h20zM12 10v5M12 17.5v.5' },
  { to: '/resources', label: 'Resource Optimizer', icon: 'M21 8 12 3 3 8v8l9 5 9-5zM3 8l9 5 9-5M12 13v8' },
  { to: '/shelters', label: 'Shelters & Victims', icon: 'M3 11 12 3l9 8M5 10v10h14V10M10 20v-6h4v6' },
];

export default function Sidebar() {
  return (
    <aside className="sidebar">
      <div className="brand">
        <svg width="30" height="30" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" strokeWidth="1.8" strokeLinejoin="round">
          <path d="M12 2 4 5.5v6c0 5 3.4 8.4 8 10.5 4.6-2.1 8-5.5 8-10.5v-6z" />
          <path d="M8.5 12.5 11 15l4.5-5" />
        </svg>
        <div>
          <strong>AEGIS</strong>
          <span>Emergency command</span>
        </div>
      </div>
      <nav>
        {links.map((l) => (
          <NavLink key={l.to} to={l.to} end={l.to === '/'} className={({ isActive }) => 'navlink' + (isActive ? ' active' : '')}>
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round">
              <path d={l.icon} />
            </svg>
            {l.label}
          </NavLink>
        ))}
      </nav>
      <p className="sidebar-foot">Simulation only. All data is invented.</p>
    </aside>
  );
}
